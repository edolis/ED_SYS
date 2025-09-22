// #region StdManifest
/**
 * @file ED_sysstd.h
 * @brief
 *
 *
 * @author Emanuele Dolis (edoliscom@gmail.com)
 * @version 0.1
 * @date 2025-08-29
 */
// #endregion

#pragma once

#include "ED_SNTP_time.h"
#include "ED_sysInfo.h"
#include "esp_netif.h"
#include "esp_wifi_types_generic.h"
#include <esp_timer.h>
#include <string>

using namespace ED_SYSINFO;

namespace ED_sysstd {

/**
 * @brief returns standardized elements used to identify and manage the ESP
 * station
 *
 */
class ESP_std {
  static constexpr size_t MAC_STRLEN = 18;
  static constexpr size_t IP_STRLEN = 16;

  static inline char _ESP_mqttID[MAC_STRLEN] = ""; // ESP_XX_XX_XX
  static inline char _ESP_NetwID[MAC_STRLEN] = ""; // ESP_XX_XX_XX
  static inline char _ESP_MAC[MAC_STRLEN] = "";    // XX:XX:XX:XX:XX:XX
  static inline char _ESP_IP[IP_STRLEN] = "";      // 000.000.000.000
  static inline char _ESP_FW_PNAME[32] = "";      //
  static inline char _ESP_FW_VER[32] = "";      // git version v0.0.0-0-xxxxxx
  static inline char _upTime[19] = "";             // 0000d 00.00.00
  static const inline esp_app_desc_t* app_desc = esp_app_get_description();

public:

  /// returns the firmware Project name
  static const char *fwPrjName() {

    return app_desc->project_name;
  }
  /// returns the firmware Project name
  static const char *fwVer() {

    return app_desc->version;
  }
  /// returns the firmware Project name
  static const char *fwDate() {

    return app_desc->date;
  }
  /// returns the standardized network name
  static const char *NetwName() {
    if (_ESP_NetwID[0] == '\0') {
      snprintf(_ESP_NetwID, sizeof(_ESP_NetwID), "ESP_%02X_%02X_%02X",
               ESP_MACstorage::getMac(esp_mac_type_t::ESP_MAC_BASE)[3],
               ESP_MACstorage::getMac(esp_mac_type_t::ESP_MAC_BASE)[4],
               ESP_MACstorage::getMac(esp_mac_type_t::ESP_MAC_BASE)[5]);
    }

    return _ESP_NetwID;
  }
  /// returns the standardized network name
  static const char *mqttName() {
    if (_ESP_mqttID[0] == '\0') {
      snprintf(_ESP_mqttID, sizeof(_ESP_mqttID), "ESP_%02X:%02X:%02X",
               ESP_MACstorage::getMac(esp_mac_type_t::ESP_MAC_BASE)[3],
               ESP_MACstorage::getMac(esp_mac_type_t::ESP_MAC_BASE)[4],
               ESP_MACstorage::getMac(esp_mac_type_t::ESP_MAC_BASE)[5]);
    }

    return _ESP_mqttID;
  }
  /// returns the standardized MAC address
  static const char *stdMAC() {
    if (_ESP_MAC[0] == '\0') {
      strcpy(_ESP_MAC, ESP_MACstorage::getMac(esp_mac_type_t::ESP_MAC_BASE)
                           .toString(_ESP_MAC, sizeof(_ESP_MAC)));
    }

    return _ESP_MAC;
  };
  static std::string curStdTime() {
    return ED_SNTP::TimeSync::getClockTime(ED_SNTP::ISOFORMAT::DATETIME_UTC_OFFSET);
  };
  // current IP address

  static const char *curIP() {
    /* lazy initialization + subscription
    allows to get the IP both in case the wifi connection already took place and
    in case the ip has not yet been received. Avoids couplig with the wifi
    management class
    */
    if (_ESP_IP[0] == '\0') {

      get_current_ip(_ESP_IP, sizeof(_ESP_IP));
      // esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED,
      // &on_wifi_connected, nullptr);
      if (!_ip_event_registered) {
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                   &ESP_std::on_ip_gotten, nullptr);
        _ip_event_registered = true;
      }
    }
    return _ESP_IP;
  };

  static const char *upTime() {
    int64_t us_since_boot = esp_timer_get_time(); // microseconds
    time_t totalSeconds = us_since_boot / 1e6;
    // uint32_t hours = totalSeconds / 3600;
    uint16_t days = totalSeconds / 86400;
    uint8_t hours = (totalSeconds % 86400) / 3600;
    uint8_t minutes = (totalSeconds % 86400 % 3600) / 60;
    uint8_t seconds = totalSeconds % 86400 % 3600 % 60;

    snprintf(_upTime, sizeof(_upTime), "%2ud %02u:%02u:%02u", days, hours,
             minutes, seconds);

    return _upTime;
  }

private:
  static inline bool _ip_event_registered = false;
  static void on_ip_gotten(void *handler_arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
      ip_event_got_ip_t *event = static_cast<ip_event_got_ip_t *>(event_data);
      esp_ip4addr_ntoa(&event->ip_info.ip, _ESP_IP, sizeof(_ESP_IP));
      ESP_LOGI("STORAGE", "IP updated to: %s", _ESP_IP);
    }
  }
  /**
   * @brief Retrieves the IP address if the Wi-Fi station is connected.
   * * @param ip_address_str A buffer to store the retrieved IP address string.
   * @param buffer_len The size of the provided buffer.
   * @return true If the Wi-Fi is connected and the IP was retrieved.
   * @return false If the Wi-Fi is not connected or an error occurred.
   */
  static bool get_current_ip(char *ip_address_str, size_t buffer_len) {

    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_default_netif();

    if (netif == NULL) {
      ESP_LOGE("GET_IP", "Default network interface not found.");
      return false;
    }

    // This is the correct and most direct way to check:
    // Try to get the IP info. If it fails, the IP is not ready.
    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
      esp_ip4addr_ntoa(&ip_info.ip, ip_address_str, buffer_len);
      ESP_LOGI("GET_IP", "IP address retrieved: %s", ip_address_str);
      return true;
    }

    ESP_LOGW("GET_IP", "No IP address assigned yet.");
    memset(ip_address_str, 0, buffer_len);
    return false;
  }

}; // class
} // namespace ED_sysstd