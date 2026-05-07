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

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <stdint.h>
#include <string>
#include <time.h>
#include <atomic>

#define TZ_EXCLUDE_AMERICA

namespace ED_SNTP {

enum class TimeZone {
  CET = 0,
  WET,
  EET,
  UK_GMT,
  EST,
  CST,
  MST,
  PST,
  AKST,
  HST,
  ARIZONA,
  SASKATCHEWAN,
  MEXICO_CITY,
  BAJA_CALIFORNIA,
  SONORA
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

static constexpr TimeZoneInfo timeZones[TIMEZONE_COUNT] = {
    {TimeZone::CET, "Central European Time", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {TimeZone::WET, "Western European Time", "WET0WEST,M3.5.0/1,M10.5.0/2"},
    {TimeZone::EET, "Eastern European Time", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {TimeZone::UK_GMT, "United Kingdom Time", "GMT0BST,M3.5.0/1,M10.5.0/2"}
#ifndef TZ_EXCLUDE_AMERICA
    ,
    {TimeZone::EST, "Eastern Time (US & Canada)", "EST5EDT,M3.2.0/2,M11.1.0/2"},
    {TimeZone::CST, "Central Time (US & Canada)", "CST6CDT,M3.2.0/2,M11.1.0/2"},
    {TimeZone::MST, "Mountain Time (US & Canada)", "MST7MDT,M3.2.0/2,M11.1.0/2"},
    {TimeZone::PST, "Pacific Time (US & Canada)", "PST8PDT,M3.2.0/2,M11.1.0/2"},
    {TimeZone::AKST, "Alaska Time", "AKST9AKDT,M3.2.0/2,M11.1.0/2"},
    {TimeZone::HST, "Hawaii Standard Time", "HST10"},
    {TimeZone::ARIZONA, "Arizona (No DST)", "MST7"},
    {TimeZone::SASKATCHEWAN, "Saskatchewan (No DST)", "CST6"},
    {TimeZone::MEXICO_CITY, "Mexico City", "CST6CDT,M4.1.0/2,M10.5.0/2"},
    {TimeZone::BAJA_CALIFORNIA, "Baja California", "PST8PDT,M4.1.0/2,M10.5.0/2"},
    {TimeZone::SONORA, "Sonora (No DST)", "MST7"}
#endif
};

static constexpr const char *NTPSERVER[] = {
    "ntp.inrim.it", "time.cloudflare.com", "europe.pool.ntp.org",
    "pool.ntp.org", "raspi00"};

enum class TICKTYPE {
  TICK_MS,
  TICK_US
};

enum class ISOFORMAT {
  DATE_ONLY,
  DATETIME_LOCAL,
  DATETIME_UTC,
  DATETIME_OFFSET,
  DATETIME_UTC_OFFSET,
  WEEK_DATE,
  ORDINAL_DATE
};

static const char* ISOFORMAT_STRINGS[] = {
    "%Y-%m-%d",
    "%Y-%m-%dT%H:%M:%S",
    "%Y-%m-%dT%H:%M:%SZ",
    "%Y-%m-%dT%H:%M:%S%z",
    "%Y-%m-%dT%H:%M:%SZ%z",
    "%G-W%V-%u",
    "%Y-%j"
};

class TimeSync {
public:
  static void initialize(const char *ntpServer = NTPSERVER[0], TimeZone tz = TimeZone::CET);
  static void initialize(uint8_t serverIndex, TimeZone tz = TimeZone::CET);

  static std::string getClockTime(ISOFORMAT format = ISOFORMAT::DATETIME_UTC_OFFSET);
  static std::string getClockTime(uint64_t rtTicks, TICKTYPE ttype = TICKTYPE::TICK_MS, ISOFORMAT format = ISOFORMAT::DATETIME_OFFSET);
  static void getClockTime(ISOFORMAT format, char *outBuf, size_t outSize);

  static uint64_t getEpochTime(uint64_t rtTicks);
  static uint64_t getEpochTime();

private:
  static std::string getClockTime_str(time_t epoch, ISOFORMAT format);
  static void getClockTime_str(time_t epoch, ISOFORMAT format, char *outBuf, size_t outSize);
  static void launchWithServer(std::string server);
  static void onTimerStatic(TimerHandle_t xTimer);
  static void initInternalTimer();
  static int8_t getSNTPserverIndex(const char *ntpServer);
  static uint8_t validateSNTPindex(uint8_t proposedIndex);
  static void setReferenceTime();
  static void syncTask(void *arg);
  static void sync_cb(struct timeval *tv);

  // Shared state protected by s_mutex
  static portMUX_TYPE s_mutex;
  static bool s_initialized;           // prevent duplicate init
  static bool s_RTCreferenceValid;
  static time_t s_referenceUnixTime;
  static uint64_t s_referenceRTC_us;
  static std::string s_curSntpServer;
  static bool s_networkAvailable;
  static bool s_espSntp_initialized;
  static bool s_initializeLaunched;    // auto‑recovery flag
  static int64_t s_startRef;
  static int64_t s_timeout_ms;
  static uint8_t s_curSNTPindex;
  static uint8_t s_numAvailableSNTP;
  static TimeZone s_referenceTimeZone;
  static TimerHandle_t s_syncTimer;
  static TaskHandle_t s_syncTaskHandle;
};

} // namespace ED_SNTP