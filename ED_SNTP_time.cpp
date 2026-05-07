#include "ED_SNTP_time.h"

#include <esp_log.h>
#include <esp_netif.h>
#include <esp_sntp.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/time.h>
#include <cstring>

namespace ED_SNTP {

static const char *TAG = "ED_SNTP_time";

// Mutex for all static variables
portMUX_TYPE TimeSync::s_mutex = portMUX_INITIALIZER_UNLOCKED;

bool TimeSync::s_initialized = false;
bool TimeSync::s_RTCreferenceValid = false;
time_t TimeSync::s_referenceUnixTime = 0;
uint64_t TimeSync::s_referenceRTC_us = 0;
std::string TimeSync::s_curSntpServer = "";
bool TimeSync::s_networkAvailable = false;
bool TimeSync::s_espSntp_initialized = false;
bool TimeSync::s_initializeLaunched = false;
int64_t TimeSync::s_startRef = -1;
int64_t TimeSync::s_timeout_ms = 1000;
uint8_t TimeSync::s_curSNTPindex = 0;
uint8_t TimeSync::s_numAvailableSNTP = sizeof(NTPSERVER) / sizeof(NTPSERVER[0]);
TimeZone TimeSync::s_referenceTimeZone = TimeZone::CET;
TimerHandle_t TimeSync::s_syncTimer = nullptr;
TaskHandle_t TimeSync::s_syncTaskHandle = NULL;

// ----------------------------------------------------------------------
// Helper: clamp SNTP index to valid range
uint8_t TimeSync::validateSNTPindex(uint8_t proposedIndex) {
  if (proposedIndex >= s_numAvailableSNTP) {
    ESP_LOGW(TAG, "Invalid SNTP index %d, resetting to 0", proposedIndex);
    return 0;
  }
  return proposedIndex;
}

int8_t TimeSync::getSNTPserverIndex(const char *ntpServer) {
  for (int8_t i = 0; i < (int8_t)s_numAvailableSNTP; i++) {
    if (strcmp(ntpServer, NTPSERVER[i]) == 0)
      return i;
  }
  return -1;
}

// ----------------------------------------------------------------------
// Public initializers
void TimeSync::initialize(uint8_t serverIndex, TimeZone tz) {
  uint8_t idx = validateSNTPindex(serverIndex);
  initialize(NTPSERVER[idx], tz);
}

void TimeSync::initialize(const char *ntpServer, TimeZone tz) {
  portENTER_CRITICAL(&s_mutex);
  if (s_initialized) {
    // Already initialized – just update timezone if changed
    if (s_referenceTimeZone != tz) {
      s_referenceTimeZone = tz;
      setenv("TZ", timeZones[static_cast<int>(tz)].POSIX.data(), 1);
      tzset();
      ESP_LOGI(TAG, "Timezone updated to %s", timeZones[(int)tz].POSIX.data());
    }
    portEXIT_CRITICAL(&s_mutex);
    return;
  }
  s_initialized = true;
  s_referenceTimeZone = tz;
  portEXIT_CRITICAL(&s_mutex);

  setenv("TZ", timeZones[static_cast<int>(tz)].POSIX.data(), 1);
  tzset();
  ESP_LOGI(TAG, "TZ set to %s", timeZones[(int)tz].POSIX.data());

  // Create sync task only once
  if (s_syncTaskHandle == NULL) {
    xTaskCreate(syncTask, "SNTP_SyncTask", 4096, NULL, 5, &s_syncTaskHandle);
  }

  launchWithServer(ntpServer);
}

// ----------------------------------------------------------------------
// SNTP server management
void TimeSync::launchWithServer(std::string server) {
  portENTER_CRITICAL(&s_mutex);
  s_RTCreferenceValid = false;
  s_espSntp_initialized = false; // will be set true after successful init
  portEXIT_CRITICAL(&s_mutex);

  if (!s_syncTimer)
    initInternalTimer();

  // Check if network interface has IP
  esp_netif_ip_info_t ip_info;
  esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  bool hasIP = (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK);

  if (hasIP) {
    portENTER_CRITICAL(&s_mutex);
    if (s_curSntpServer != server) {
      if (s_espSntp_initialized) {
        esp_sntp_stop();
        s_espSntp_initialized = false;
      }
      s_curSntpServer = server;
      // Update index if server is in our list
      int idx = getSNTPserverIndex(server.c_str());
      s_curSNTPindex = (idx >= 0) ? (uint8_t)idx : 0;
      portEXIT_CRITICAL(&s_mutex);

      esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
      esp_sntp_setservername(0, s_curSntpServer.c_str());
      esp_sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
      esp_sntp_set_time_sync_notification_cb(sync_cb);
      esp_sntp_init();

      portENTER_CRITICAL(&s_mutex);
      s_espSntp_initialized = true;
      portEXIT_CRITICAL(&s_mutex);
    }
    portENTER_CRITICAL(&s_mutex);
    s_networkAvailable = true;
    portEXIT_CRITICAL(&s_mutex);
    xTimerStart(s_syncTimer, 0);
    ESP_LOGI(TAG, "SNTP configured for server: %s", s_curSntpServer.c_str());
  } else {
    portENTER_CRITICAL(&s_mutex);
    s_networkAvailable = false;
    portEXIT_CRITICAL(&s_mutex);
    // Still start timer – it will check again later
    xTimerStart(s_syncTimer, 0);
    ESP_LOGW(TAG, "No IP, will retry SNTP later");
  }
}

// ----------------------------------------------------------------------
// Timer callback (runs in timer task context)
void TimeSync::onTimerStatic(TimerHandle_t xTimer) {
  portENTER_CRITICAL(&s_mutex);
  bool netAvail = s_networkAvailable;
  bool sntpInit = s_espSntp_initialized;
  int64_t now = esp_timer_get_time();
  int64_t start = s_startRef;
  int64_t timeout = s_timeout_ms;
  std::string curServer = s_curSntpServer;
  portEXIT_CRITICAL(&s_mutex);

  if (!netAvail) {
    // No IP – nothing we can do, just wait for next timer tick
    return;
  }

  if (start == -1) {
    portENTER_CRITICAL(&s_mutex);
    s_startRef = now;
    portEXIT_CRITICAL(&s_mutex);
  }

  if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
    // Sync done – stop timer, sync task will be notified via callback
    xTimerStop(s_syncTimer, 0);
    portENTER_CRITICAL(&s_mutex);
    s_startRef = -1;
    portEXIT_CRITICAL(&s_mutex);
    return;
  }

  // Timeout: switch to next server
  if ((now - start) > (timeout * 1000)) {
    portENTER_CRITICAL(&s_mutex);
    s_startRef = -1;
    // Find next server index
    int8_t curIdx = getSNTPserverIndex(curServer.c_str());
    if (curIdx < 0) curIdx = 0;
    uint8_t nextIdx = (curIdx + 1) % s_numAvailableSNTP;
    // Increase timeout for next attempt
    s_timeout_ms += 500;
    std::string nextServer = NTPSERVER[nextIdx];
    portEXIT_CRITICAL(&s_mutex);

    ESP_LOGW(TAG, "Timeout connecting to %s, switching to %s (timeout=%lld ms)",
             curServer.c_str(), nextServer.c_str(), s_timeout_ms);
    launchWithServer(nextServer);
  }
}

void TimeSync::initInternalTimer() {
  if (s_syncTimer == nullptr) {
    s_syncTimer = xTimerCreate("SNTPsync_timer", pdMS_TO_TICKS(200), pdTRUE,
                               nullptr, onTimerStatic);
  }
}

// ----------------------------------------------------------------------
// Reference time capture (called from syncTask)
void TimeSync::setReferenceTime() {
  time_t now = 0;
  time(&now);
  int64_t rtc_now = esp_timer_get_time();

  portENTER_CRITICAL(&s_mutex);
  s_referenceUnixTime = now;
  s_referenceRTC_us = rtc_now;
  s_RTCreferenceValid = true;
  portEXIT_CRITICAL(&s_mutex);

  tzset();

  std::string clocktime = getClockTime();
  ESP_LOGI(TAG, "Reference captured: Unix=%lld, RTC=%lld, local time=%s",
           (long long)now, (long long)rtc_now, clocktime.c_str());
}

// ----------------------------------------------------------------------
// SNTP callback (ISR context – only sends notify)
void TimeSync::sync_cb(struct timeval *tv) {
  ESP_LOGI("SNTP", "Time synchronized: %ld", tv->tv_sec);
  if (s_syncTaskHandle != NULL) {
    xTaskNotifyGive(s_syncTaskHandle);
  }
}

// ----------------------------------------------------------------------
// Sync task – does the heavy lifting after sync completes
void TimeSync::syncTask(void *arg) {
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    setReferenceTime();

    portENTER_CRITICAL(&s_mutex);
    if (s_espSntp_initialized) {
      esp_sntp_stop();
      s_espSntp_initialized = false;
      s_timeout_ms = 1000;       // reset timeout for next sync
      s_initializeLaunched = false; // allow auto-retry later if needed
    }
    int64_t elapsed = (esp_timer_get_time() - s_startRef) / 1000;
    portEXIT_CRITICAL(&s_mutex);

    ESP_LOGI(TAG, "Sync completed with %s in %lld ms", s_curSntpServer.c_str(), (long long)elapsed);
  }
}

// ----------------------------------------------------------------------
// Public time getters
std::string TimeSync::getClockTime(ISOFORMAT format) {
  portENTER_CRITICAL(&s_mutex);
  bool valid = s_RTCreferenceValid;
  bool launched = s_initializeLaunched;
  portEXIT_CRITICAL(&s_mutex);

  if (!valid) {
    if (!launched) {
      ESP_LOGW(TAG, "No SNTP reference, launching sync");
      initialize();
      portENTER_CRITICAL(&s_mutex);
      s_initializeLaunched = true;
      portEXIT_CRITICAL(&s_mutex);
    } else {
      ESP_LOGW(TAG, "No SNTP reference yet");
    }
    return "- no valid clock on ESP -";
  }
  time_t now = time(nullptr);
  return getClockTime_str(now, format);
}

void TimeSync::getClockTime(ISOFORMAT format, char* outBuf, size_t outSize) {
  if (!outBuf || outSize == 0) return;

  portENTER_CRITICAL(&s_mutex);
  bool valid = s_RTCreferenceValid;
  bool launched = s_initializeLaunched;
  portEXIT_CRITICAL(&s_mutex);

  if (!valid) {
    if (!launched) {
      ESP_LOGW(TAG, "No SNTP reference, launching sync");
      initialize();
      portENTER_CRITICAL(&s_mutex);
      s_initializeLaunched = true;
      portEXIT_CRITICAL(&s_mutex);
    } else {
      ESP_LOGW(TAG, "No SNTP reference yet");
    }
    snprintf(outBuf, outSize, "- no valid clock on ESP -");
    return;
  }
  time_t now = time(nullptr);
  getClockTime_str(now, format, outBuf, outSize);
}

std::string TimeSync::getClockTime(uint64_t rtTicks, TICKTYPE ttype, ISOFORMAT format) {
  portENTER_CRITICAL(&s_mutex);
  bool valid = s_RTCreferenceValid;
  uint64_t refRTC = s_referenceRTC_us;
  time_t refUnix = s_referenceUnixTime;
  portEXIT_CRITICAL(&s_mutex);

  if (!valid || refUnix == 0 || refRTC == 0) {
    if (!s_initializeLaunched) {
      ESP_LOGW(TAG, "No SNTP reference, launching sync");
      initialize();
      portENTER_CRITICAL(&s_mutex);
      s_initializeLaunched = true;
      portEXIT_CRITICAL(&s_mutex);
    } else {
      ESP_LOGW(TAG, "No SNTP reference, returning invalid");
    }
    return "- no valid clock on ESP -";
  }

  uint64_t deltaMicros;
  if (ttype == TICKTYPE::TICK_US) {
    deltaMicros = rtTicks - refRTC;
  } else {
    deltaMicros = (rtTicks * 1000ULL) - refRTC;
  }
  time_t epoch = refUnix + (deltaMicros / 1000000ULL);
  return getClockTime_str(epoch, format);
}

uint64_t TimeSync::getEpochTime(uint64_t rtTicks) {
  portENTER_CRITICAL(&s_mutex);
  bool valid = s_RTCreferenceValid;
  uint64_t refRTC = s_referenceRTC_us;
  time_t refUnix = s_referenceUnixTime;
  portEXIT_CRITICAL(&s_mutex);

  if (!valid || refUnix == 0 || refRTC == 0) {
    if (!s_initializeLaunched) {
      ESP_LOGW(TAG, "No SNTP reference, launching sync");
      initialize();
      portENTER_CRITICAL(&s_mutex);
      s_initializeLaunched = true;
      portEXIT_CRITICAL(&s_mutex);
    } else {
      ESP_LOGW(TAG, "No SNTP reference, returning 0");
    }
    return 0;
  }
  uint64_t deltaMicros = rtTicks - refRTC;
  return refUnix + (deltaMicros / 1000000ULL);
}

uint64_t TimeSync::getEpochTime() {
  return getEpochTime(esp_timer_get_time());
}

// ----------------------------------------------------------------------
// String formatting helpers (fixed epoch overwrite bug)
std::string TimeSync::getClockTime_str(time_t epoch, ISOFORMAT format) {
  char buffer[64];
  getClockTime_str(epoch, format, buffer, sizeof(buffer));
  return std::string(buffer);
}

void TimeSync::getClockTime_str(time_t epoch, ISOFORMAT format, char* outBuf, size_t outSize) {
  if (!outBuf || outSize == 0) return;

  // Use provided epoch, do NOT overwrite with time(NULL)
  struct tm timeinfo;
  if (format == ISOFORMAT::DATETIME_UTC || format == ISOFORMAT::DATETIME_UTC_OFFSET) {
    gmtime_r(&epoch, &timeinfo);
  } else {
    localtime_r(&epoch, &timeinfo);
  }

  strftime(outBuf, outSize, ISOFORMAT_STRINGS[static_cast<int>(format)], &timeinfo);

  if (format == ISOFORMAT::DATETIME_UTC_OFFSET) {
    // Correct offset handling: replace wrong %z with local offset
    struct tm localInfo;
    char tzOffsStr[7] = "";
    localtime_r(&epoch, &localInfo);
    strftime(tzOffsStr, sizeof(tzOffsStr), "%z", &localInfo);
    size_t len = strlen(outBuf);
    if (len > 5) {
      outBuf[len - 5] = '\0';   // trim the incorrect Z+0000 part
    }
    strncat(outBuf, tzOffsStr, outSize - strlen(outBuf) - 1);
  }
}

} // namespace ED_SNTP