#include "ED_SNTP_time.h"

#include <esp_log.h>
#include <esp_netif.h>
#include <esp_sntp.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/time.h>

namespace ED_SNTP {
static const char *TAG = "ED_SNTP_time";
static portMUX_TYPE rtc_mux = portMUX_INITIALIZER_UNLOCKED;

bool TimeSync::RTCreferenceValid = false;
time_t TimeSync::referenceUnixTime = 0;
uint64_t TimeSync::referenceRTC = 0;
uint8_t TimeSync::curSNTPindex = 0;
std::string TimeSync::curSntpServer = "";

uint8_t TimeSync::numAvailableSTNP =
    sizeof(NTPSERVER) /
    sizeof(NTPSERVER[0]); // captures the size of the array programmatically

int8_t TimeSync::getSNTPserverIndex(const char *ntpServer) {
  for (int8_t i = 0; i < numAvailableSTNP; i++) {
    if (strcmp(ntpServer, NTPSERVER[i]) == 0)
      return i;
  }
  return -1; // nopt found
}
uint8_t TimeSync::validateSNTPindex(uint8_t proposedIndex) {
  if (proposedIndex >= numAvailableSTNP)
  { //increases the threshold for timeout to facilitate
    ESP_LOGI(TAG,"Could not connect SNTP servers with %dms timeout. Trying again with %dms ", timeout_ms, timeout_ms+500);
    timeout_ms+=500;
    return 0;}
  return proposedIndex;
}
void TimeSync::initialize(uint8_t serverIndex, TimeZone tz) {
  uint8_t validatedIndex = validateSNTPindex(serverIndex);
  referenceTimeZone = tz;

  initialize(NTPSERVER[validatedIndex - 1], tz);
}

void TimeSync::initialize(const char *ntpServer, TimeZone tz) {

  // consumes the timezone info to configure the time conversion
  setenv("TZ", timeZones[static_cast<int>(referenceTimeZone)].POSIX.data(), 1);
  tzset();

  xTaskCreate(syncTask, "SNTP_SyncTask", 2048, NULL, 5, &syncTaskHandle);
  launchWithServer(ntpServer);
}

void TimeSync::launchWithServer(std::string server) {
  RTCreferenceValid = false; // in case initialize is called after a successful
                             // run and now network unavailable

  if (!syncTimer)
    initInternalTimer();
  xTimerStart(syncTimer, 0); // timer takes care also of network
  //  subsequent calls to timerstart reset the timer

  esp_netif_ip_info_t ip_info;
  esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

  if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
    // ESP_LOGI(TAG, "TRACE-5 in launchWithServer");

    // IP is assigned, safe to start SNTP
    if (curSntpServer != server) {
      //   ESP_LOGI(TAG, "TRACE-6 in launchWithServer");

      if (espSntp_initialized) {
        esp_sntp_stop();
        espSntp_initialized = false;
      }

      esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
      esp_sntp_setservername(0, curSntpServer.c_str());

      // Register the callback BEFORE init
      esp_sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED); // Immediate notification
      esp_sntp_set_time_sync_notification_cb(sync_cb);

      esp_sntp_init();

      espSntp_initialized = true;
      curSntpServer = server;
    }

    networkAvailable = true; // used by the timer callback
  } else {
    networkAvailable =
        false; // needs be set again in case called after a previous successful
               // initialization. network might have become unavailable
    // hnetwork not ready
    //  returns and uses the timer to schedule another attempt
    return;
  }
  //

  ESP_LOGI("TimeSync", "SNTP configured for connection to: %s",
           curSntpServer.c_str());
};

// void TimeSync::setReferenceRTC(uint64_t value) {
//   portENTER_CRITICAL(&rtcMux);
//   referenceRTC = value;
//   portEXIT_CRITICAL(&rtcMux);
// }

uint64_t TimeSync::getReferenceRTC() {
  uint64_t value;
  portENTER_CRITICAL(&rtc_mux);
  value = referenceRTC;
  portEXIT_CRITICAL(&rtc_mux);
  return value;
}

void TimeSync::setReferenceTime() {

  //   ESP_LOGI(TAG, "in setReferenceTime");
  time_t now = 0;
  time(&now);
  portENTER_CRITICAL_ISR(&rtc_mux);
  referenceUnixTime = now;
  portEXIT_CRITICAL_ISR(&rtc_mux);
  int64_t referenceRTC_test_local = esp_timer_get_time();
  portENTER_CRITICAL_ISR(&rtc_mux);
  referenceRTC = referenceRTC_test_local;
  portEXIT_CRITICAL_ISR(&rtc_mux);
  std::string clocktime = getClockTime();

  ESP_LOGI("TimeSync",
           "Reference captured: Unix=%" PRIu64 ", RTSC=%" PRIu64
           " at local clock time %s",
           (uint64_t)now, (uint64_t)referenceRTC_test_local, clocktime.c_str());

  RTCreferenceValid = true;
}

void TimeSync::onTimerStatic(TimerHandle_t xTimer) {
  // checks timeoutKTRACE_in in ontimerstatic");

  //   const int64_t timeout_ms = 2000;
  //   static int64_t startRef = -1; // unset
  int8_t newServerIndex = -1;
  if (networkAvailable) {
    // ESP_LOGI(TAG,"TRACEOKNET in ontimerstatic");
    if (startRef == -1)
      startRef = esp_timer_get_time(); // allows using the timer multiple times,
                                       // which requires a reset

    if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
      // setReferenceTime(); ==> moved to the task, as it is too heavy and
      // causes stack overflow

      xTimerStop(syncTimer,
                 0); // TODO implement periodic reconnection for alignment on
      // quite long interval, like day? and also check that the
      // client is not doing that itself

      startRef = -1; // resets the reference time to unassigned

      return;
    } else if ((esp_timer_get_time() - startRef) > timeout_ms * 1000) {
      startRef = -1;
      newServerIndex =
          validateSNTPindex(getSNTPserverIndex(curSntpServer.c_str()) + 1);

      ESP_LOGW(TAG, "Failed connecting %s. Switching SNTP to %s.",
               curSntpServer.c_str(), NTPSERVER[newServerIndex]);

      launchWithServer(NTPSERVER[newServerIndex]);
    }
  } else
    launchWithServer(curSntpServer);
};

void TimeSync::initInternalTimer() {

  syncTimer = xTimerCreate("SNTPsync_timer",   // Timer name
                           pdMS_TO_TICKS(200), // Period in ticks (e.g. 1000 ms)
                           pdTRUE,             // Auto-reload
                           nullptr,            // Timer ID (optional)
                           TimeSync::onTimerStatic // Callback function
  );
};

std::string TimeSync::getClockTime(uint64_t rtTicks) {
  time_t unixTime = getUTCUnixTime(rtTicks);
  ;
  struct tm *localTime = localtime(&unixTime);
  char buffer[64];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localTime); // format:  2025-08-26 23:31:40
  return std::string(buffer);
};

std::string TimeSync::getClockTime() {
  if (!RTCreferenceValid) {
    ESP_LOGW(TAG, "SNTP not initialized. Returning empty time.");
    return "";
  }
  time_t now;
  time(&now);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  char buffer[64];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return std::string(buffer);
}

uint64_t TimeSync::getUTCUnixTime(uint64_t rtTicks) {
  int64_t getReferenceRTC_local =
      getReferenceRTC(); // makes local copy to minimize access to the static
                         // member and potentially creating conflicts with the
                         // timer execution
  // if (!RTCreferenceValid) {
  //   ESP_LOGW(TAG, "SNTP not initialized. Returning 0.");
  //   return 0;
  // }
  if (referenceUnixTime == 0 || getReferenceRTC_local == 0) {
    ESP_LOGW("TimeSync", "Reference time not set. Returning 0.");
    return 0;
  }

  //   int64_t deltaMicros = rtTicks - referenceRTC;
  int64_t deltaMicros = rtTicks - getReferenceRTC_local;
  return referenceUnixTime + (deltaMicros / 1000000ULL);
}

uint64_t TimeSync::getUTCUnixTime() {
  return getUTCUnixTime(esp_timer_get_time());
}

void TimeSync::syncTask(void *arg) {
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for sync signal
    setReferenceTime();                      // Your RTC update logic
    if (espSntp_initialized) { // sntp completed, releases the resources
      esp_sntp_stop();
      espSntp_initialized = false;
      timeout_ms=1000;
    }
    int elapsedSec = (esp_timer_get_time() - startRef) / 1e3;
    ESP_LOGI(TAG, "Sync with server %s completed in %d ms",
             curSntpServer.c_str(), elapsedSec);
  }
}

void TimeSync::sync_cb(struct timeval *tv) {
  ESP_LOGI("SNTP", "Time synchronized: %ld", tv->tv_sec);
  if (syncTaskHandle != NULL) {
    xTaskNotifyGive(syncTaskHandle); // Signal the task
  }
}

} // namespace ED_SNTP