#pragma once

// #region StdManifest
/**
 * @file ED_SNTP_time.h
 * @brief synchronizes the clock time to the internal ESP32 RTC, using SNTP
 *
 *
 * @author Emanuele Dolis (edoliscom@gmail.com)
 * @version 0.1
 * @date 2025-08-25
 */
// #endregion

//#include "freertos/timers.h"
#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
#include "freertos/timers.h"
#include <stdint.h>
#include <string>
#include <time.h>
#include <atomic>

#define TZ_EXCLUDE_AMERICA

namespace ED_SNTP {

// #region POSIXmap

enum class TimeZone {
  CET = 0,         // Central European Time
  WET,             // Western European Time
  EET,             // Eastern European Time
  UK_GMT,          // United Kingdom Time
  EST,             // Eastern Time (US & Canada)
  CST,             // Central Time (US & Canada)
  MST,             // Mountain Time (US & Canada)
  PST,             // Pacific Time (US & Canada)
  AKST,            // Alaska Time
  HST,             // Hawaii Standard Time
  ARIZONA,         // Arizona (No DST)
  SASKATCHEWAN,    // Saskatchewan (No DST)
  MEXICO_CITY,     // Mexico City
  BAJA_CALIFORNIA, // Baja California
  SONORA           // Sonora (No DST)
};

struct TimeZoneInfo {
  TimeZone zone;
  std::string_view readableId;
  std::string_view POSIX;
};

#ifdef TZ_EXCLUDE_AMERICA
#define TIMEZONE_COUNT 4
#else
#define TIMEZONE_COUNT 15
#endif
// Timezone description and POSIX
static constexpr TimeZoneInfo timeZones[TIMEZONE_COUNT] = {
    {TimeZone::CET, "Central European Time", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {TimeZone::WET, "Western European Time", "WET0WEST,M3.5.0/1,M10.5.0/2"},
    {TimeZone::EET, "Eastern European Time", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {TimeZone::UK_GMT, "United Kingdom Time", "GMT0BST,M3.5.0/1,M10.5.0/2"}

#ifndef TZ_EXCLUDE_AMERICA
,
    {TimeZone::EST, "Eastern Time (US & Canada)", "EST5EDT,M3.2.0/2,M11.1.0/2"},
    {TimeZone::CST, "Central Time (US & Canada)", "CST6CDT,M3.2.0/2,M11.1.0/2"},
    {TimeZone::MST, "Mountain Time (US & Canada)",
     "MST7MDT,M3.2.0/2,M11.1.0/2"},
    {TimeZone::PST, "Pacific Time (US & Canada)", "PST8PDT,M3.2.0/2,M11.1.0/2"},
    {TimeZone::AKST, "Alaska Time", "AKST9AKDT,M3.2.0/2,M11.1.0/2"},
    {TimeZone::HST, "Hawaii Standard Time", "HST10"},
    {TimeZone::ARIZONA, "Arizona (No DST)", "MST7"},
    {TimeZone::SASKATCHEWAN, "Saskatchewan (No DST)", "CST6"},
    {TimeZone::MEXICO_CITY, "Mexico City", "CST6CDT,M4.1.0/2,M10.5.0/2"},
    {TimeZone::BAJA_CALIFORNIA, "Baja California",
     "PST8PDT,M4.1.0/2,M10.5.0/2"},
    {TimeZone::SONORA, "Sonora (No DST)", "MST7"}
#endif

};

// #endregion POSIXmap

// SNTP server list, in order of performance as reached from Italy
#ifdef DEBUG_BUILD
//injects wrong hostnames for testing
static constexpr const char *NTPSERVER[] = {
    "xxxxntp.inrim.it", "xxxtime.cloudflare.com", "europe.pool.ntp.org",
    "pool.ntp.org", "raspi00"}; // adds the intranet SNTP server
    #else
static constexpr const char *NTPSERVER[] = {
    "ntp.inrim.it", "time.cloudflare.com", "europe.pool.ntp.org",
    "pool.ntp.org", "raspi00"}; // adds the intranet SNTP server
#endif



/**
 * @brief Provides Clock time services by aligning the ESP32 RTC data to the
 * clock time using NTP servers. Includesa a intranet SNTP as a fallback when
 * internet is not avaiilable
 *
 */
class TimeSync {
public:
  /**
   * @brief initializes the SNTP using the specified SNTP server hostname.
   * equivalent to call to initialize(0)
   *
   * @param ntpServer in the hostname or IP address
   * @param tz TimeZone identifier used to convert the UTC to local
   */
  static void initialize(const char *ntpServer = NTPSERVER[0],TimeZone tz= TimeZone::CET );
  /**
   * @brief initializes the SNTP using the static array, picking the item at the
   * specified index. Tentatively, in order of performance. 4 elements in the
   * array
   *
   * @param serverIndex between 0 and 3. Bigger values return the default
   * (equivalent to 0)
   * @param tz TimeZone identifier used to convert the UTC to local
   * timezone/daylightsaving setting
   */
  static void initialize(uint8_t serverIndex, TimeZone tz = TimeZone::CET);
//   static bool waitForSync(int timeoutSeconds = 10);
  // local clock time, according to the set timezone. conforming both to
  // timezone and its daylight saving settings
  static std::string getClockTime();
  /**
   * @brief returns the UTC time in Unix format,
   *
   * @param rtTicks the ESP32 RTC ticks from ESP internal RTC clock
   * @return uint64_t the Unix representation of UTC code
   */
  static uint64_t getUTCUnixTime(uint64_t rtTicks);
  /**
   * @brief returns the current UTC time in Unix format,
   *
   * @return uint64_t the Unix representation of UTC current time
   */
  static uint64_t getUTCUnixTime();
  /**
   * @brief returns the clock time (implementing timezone and daylight settings)
   * corresponding to the provided RTC time clicks
   *
   * @param rtTicks the ESP32 RTC ticks from ESP internal RTC clock
   * @return the string representation of the clock time
   */
  static std::string getClockTime(uint64_t rtTicks);

private:
 static void launchWithServer(std::string server);
//   TimeSync() =
//       default; // class is meant to be used as static singleton, this is just to
//                // create a hidden instance to use with the timer
//   static TimeSync &getInstance() {
//     static TimeSync instance;
//     return instance;
//   };
static  void onTimerStatic(TimerHandle_t xTimer);
//   static void onTimerStatic(void *arg) {
//     getInstance().tick(); // delegate to instance method
//   }
static std::string curSntpServer;
static inline bool networkAvailable=false;

static inline TimerHandle_t syncTimer=nullptr; // non static as need to be a property of the
static void initInternalTimer();
// checks if the provided server name matches one of the predefined ones,
// returns -1 if not matched
static int8_t getSNTPserverIndex(const char *ntpServer);

// valideated the proposed index so that stays within the range of the array
// of SNTP (defined in NTPSERVER)
static uint8_t validateSNTPindex(uint8_t proposedIndex);

static uint8_t numAvailableSTNP;
static uint8_t curSNTPindex;
static inline TimeZone referenceTimeZone;
static bool RTCreferenceValid;
static inline bool espSntp_initialized=false;
static time_t referenceUnixTime;
//  static uint64_t referenceRTC;
 static uint64_t referenceRTC;
//getter to access safely ReferenceRTC in multithread mode. minimizes the time to get the value under portxxx_CRITICAL mux
static uint64_t getReferenceRTC() ;
static void setReferenceTime();

static inline TaskHandle_t syncTaskHandle = NULL;
static void syncTask(void *arg) ;
static void sync_cb(struct timeval *tv) ;
static inline int64_t startRef = -1;
static  inline int64_t timeout_ms = 1000;

};

} // namespace ED_SNTP