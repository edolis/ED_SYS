// #region StdManifest
/**
 * @file SNTP_time.cpp
 * @brief shows the basic setting to gonnect a SNTP server
 * using the EED_SNTP::TimeSync and then using the
 * static singleton to get clock time and unixtime
 * resolution services for RTC Ticks
 *
 *
 * @author Emanuele Dolis (edoliscom@gmail.com)
 * @version GIT_VERSION: v1.0.0.0-0-dirty
 * @tagged as: SNTP-core
 * @commit hash: g5d100c9 [5d100c9e7fbf8030cd9e50ec7db3b7b6333dbee1]
 * @build ID: P20250827-001704-5d100c9
 *
 * @date 2025-08-25
 */

static const char *TAG = "ESP_main_loop";

// #region BuildInfo
namespace ED_SYSINFO {
// compile time GIT status
struct GIT_fwInfo {
  static constexpr const char *GIT_VERSION = "v1.0.0.0-0-dirty";
  static constexpr const char *GIT_TAG = "SNTP-core";
  static constexpr const char *GIT_HASH = "g5d100c9";
  static constexpr const char *FULL_HASH =
      "5d100c9e7fbf8030cd9e50ec7db3b7b6333dbee1";
  static constexpr const char *BUILD_ID = "P20250825-162848-5d100c9";
};
} // namespace ED_SYSINFO
// #endregion
// #endregion

#include "ED_SNTP_time.h"
#include "ED_sysInfo.h"
#include "ED_wifi.h"
// #include "esp_partition.h"
#include <string.h>

#ifdef DEBUG_BUILD
#endif

using namespace ED_SNTP;

void launchSNTPalignment() {

  ESP_LOGI(TAG, "Starting time sync...");


  // Step 1: Initialize SNTP
//   TimeSync::initialize(TimeZone::CET); //...using default server
  TimeSync::initialize("raspi00",TimeZone::CET); //...using a specific server
  // Step 2: Wait for synchronization
    //NO wait-> moved to asynch execution
    // Step 3: Get formatted time
    //starts purposedly before the IP received event to show the behavior when no SNTP reference data are available
    std::string currentTime = TimeSync::getClockTime();
    ESP_LOGI(TAG, "Current time: %s", currentTime.c_str());

    // Step 4: Simulate RTSC conversion
    uint64_t nowRTSC = esp_timer_get_time(); // microseconds since boot
    uint64_t unixTime = TimeSync::getUTCUnixTime(nowRTSC);
    ESP_LOGI(TAG, "RTSC converted to Unix time: %llu", unixTime);
};


extern "C" void app_main(void) {
#ifdef DEBUG_BUILD

  if (CONFIG_LOG_MAXIMUM_LEVEL > CONFIG_LOG_DEFAULT_LEVEL) {
    esp_log_level_set("wifi", (esp_log_level_t)CONFIG_LOG_MAXIMUM_LEVEL);
  }
#endif

  ED_wifi::WiFiService::subscribeToIPReady(launchSNTPalignment);
  ED_wifi::WiFiService::launch();

  // Optional hardware setup
  // gpio_set_direction(LED_BUILTIN, GPIO_MODE_OUTPUT);

#ifdef DEBUG_BUILD

#endif

  while (true) {
    // shows the results of the first calls which will happen when network not
    // initialized, afterwards calls will get froper feedback
    uint64_t nowRTSC = esp_timer_get_time(); // microseconds since boot
    uint64_t unixTime = TimeSync::getUTCUnixTime(nowRTSC);
    std::string fmtdTime = TimeSync::getClockTime();
    ESP_LOGI(TAG, "RTSC converted to Unix time: %llu  formatted: %s", unixTime,
             fmtdTime.c_str());
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}
