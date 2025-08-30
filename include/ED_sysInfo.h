#pragma once

#include "ED_json.h" // to grab the interface specs
#include "esp_app_desc.h"
/*
BT_ENABLED must be defined in platformio ini as environment build flag, to
enable nluetooth operations, e.g. [env:main_BT] build_flags= -D BT_ENABLED -D
DEBUG_MODE

besides this conditional compilation, if is used by the pre-build script
merge_sdkconfig to pull in the sdkconfig the required DT components. Bluetooth
is conditional as it pulls the headers required to monitor the bluetooth
activity and realign the MAC address in the rare case the MAC base address has
been changed by the user. if BT_ENABLED is not defined, the Bluetooth MAC is the
one calculated on the base of the factory base MAC OR the efuse override by the
user OR by the software nase MAC set by Wifi

*/
#ifdef BT_ENABLED
#include "esp_bt_defs.h" // For Bluetooth status codes like ESP_BT_STATUS_SUCCESS
#include "esp_bt_main.h"     // For initializing and enabling Bluedroid
#include "esp_gap_ble_api.h" // For GAP BLE events and callback registration
#endif
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_mac.h" // For ESP_MAC_BT and MAC address handling
#include "esp_system.h"
#include <esp_event.h>
#include <esp_partition.h>
#include <esp_wifi.h>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>


static constexpr esp_mac_type_t ESP_MAC_NOTSET =
    static_cast<esp_mac_type_t>(-1);

namespace ED_SYSINFO {

/**
 * @brief Firmware iNFO singleton class returning firmware info. it just wraps
 * up the sorce data in const pointers, the caller has to cache as needed
 */
class fwInfo {
public:
  struct VersionInfo {
    int major = 0; // no backward compatibility on change
    int minor = 0; // backward compatibility ensured on change
    int patch = 0; // bugfix, no functional change
    int commitsAhead =
        0; // number of commits of local code ahead of remote GIT storage
    bool isDirty = false; // true if the firmware was compiled ahead of commit
    bool isValidGIT = false; // true if conforms to GIT format
    std::string full;
    std::string GITcommitID;
    bool isEmpty() { return full.length() == 0; };
  };

public:
  /// project/application name
  static const char *PrjName();

  static const char *AppCompileDate();

  static const VersionInfo &AppVers();

  static const char *AppCompileTime();

  static const char *AppELFSha256();

  static const char *AppESPIDF();

private:
  /// Version, following GIT pattern e.g. v1.12.3-2-gabc1234-dirty
  static const char *_AppVersStr();

  static VersionInfo &parseVersion(const char *versionStr);
  static VersionInfo *_Vinfo;
  static const esp_app_desc_t *getDesc();
};

//   log_app_info() {

//     const esp_app_desc_t* app_desc = esp_app_get_description();
//     ESP_LOGI(TAG, "Project name:     %s", app_desc->project_name);
//     ESP_LOGI(TAG, "App version:      %s", app_desc->version);
//     ESP_LOGI(TAG, "Compile time:     %s %s", app_desc->date, app_desc->time);
//     ESP_LOGI(TAG, "ELF file SHA256:  %s", app_desc->app_elf_sha256);
//     ESP_LOGI(TAG, "ESP-IDF:          %s", esp_get_idf_version());
// }

static const char *esp_mac_type_str[] = {
    /* ESP_MAC_WIFI_STA */ "ESP_MAC_WIFI_STA", /**< MAC for WiFi Station (6
                                                  bytes) */
    /* ESP_MAC_WIFI_SOFTAP */ "ESP_MAC_WIFI_SOFTAP", /**< MAC for WiFi Soft-AP
                                                        (6 bytes) */
    /* ESP_MAC_BT */ "ESP_MAC_BT",     /**< MAC for Bluetooth (6 bytes) */
    /* ESP_MAC_ETH */ "ESP_MAC_ETH",   /**< MAC for Ethernet (6 bytes) */
                                       /* ESP_MAC_IEEE802154 */
    "ESP_MAC_IEEE802154",              /**< if
                                          CONFIG_SOC_IEEE802154_SUPPORTED=y,
                                          MAC for IEEE802154 (8
                                          bytes) */
    /* ESP_MAC_BASE */ "ESP_MAC_BASE", /**< Base MAC for that used for other MAC
                                          types (6 bytes) */
    /* ESP_MAC_EFUSE_FACTORY */ "ESP_MAC_EFUSE_FACTORY", /**< MAC_FACTORY eFuse
                                                            which was burned by
                                                            Espressif in
                                                            production (6 bytes)
                                                          */
    /* ESP_MAC_EFUSE_CUSTOM */ "ESP_MAC_EFUSE_CUSTOM",   /**< MAC_CUSTOM eFuse
                                                            which was can be
                                                            burned by customer (6
                                                            bytes) */
                                                         /* ESP_MAC_EFUSE_EXT */
    "ESP_MAC_EFUSE_EXT"                                  /**< if
                                                            CONFIG_SOC_IEEE802154_SUPPORTED=y,
                                                            MAC_EXT eFuse which is used
                                                            as an extender for IEEE802154
                                                            MAC (2 bytes) */
};

class MacAddress {
public:
  // Default constructor (initializes to all zeros)
  MacAddress() { memset(_mac_addr, 0, 6); }

  // Constructor to initialize with a C-style array
  MacAddress(const uint8_t mac[6]) : MacAddress() { set(mac); }

  // For read-only access on a const object
  const uint8_t &operator[](size_t index) const { return _mac_addr[index]; }
  // Get a pointer to the internal array
  const uint8_t *get() const { return _mac_addr; }
  // returns a pointer to the internal class allowing modifications.
  uint8_t *set(const uint8_t mac[6]) {
    if (!isZeroMac(mac)) {
      _isValid = true;
      memcpy(_mac_addr, mac, 6);
    }
    return _mac_addr;
  }
  // Formats the MAC address into a provided buffer.
  // The buffer must be at least 18 characters long.
  char *toString(char *buffer, size_t buffer_size, char separator = ':') const {
    if (buffer && buffer_size >= 18) {
      snprintf(buffer, buffer_size, "%02X%c%02X%c%02X%c%02X%c%02X%c%02X",
               _mac_addr[0], separator, _mac_addr[1], separator, _mac_addr[2],
               separator, _mac_addr[3], separator, _mac_addr[4], separator,
               _mac_addr[5]

      );
    };
    return buffer;
  };
  // returns false if MAC is all zeroes.
  bool isValid() const { return _isValid; }

private:
  bool isZeroMac(const uint8_t mac[6]) const {
    return memcmp(mac, zeroMac, 6) == 0;
  }
  static inline const uint8_t zeroMac[6] = {
      0}; // zeroed MAC used as a reference to detect invalidity
  uint8_t _mac_addr[6];
  bool _isValid = false;
};

/**
 * @brief storage class for ESP MAC addresses,
 * with option to track potential changes at network connection
 * due to code overrifing base MAC
 *
 */
class ESP_MACstorage {
private:
  static std::mutex macMapMutex;
  ESP_MACstorage() =
      delete; // singleton. in case initlistener should become default
              // behaviour, change to hidden generation of an instance
  // which calls the function in its instance constructor

  // the storage map for the MAC of the device
  static std::unordered_map<esp_mac_type_t, MacAddress> ESPmacMap;

public:
  /// @brief  initializes the listeners which verify, at connection with Wifi or
  /// Bluetooth,
  // whether the base MAC has been overridden by the user.
  // call the method only if monitoring change of base address by obscure module
  // is needed
  static void initListener();

  // returns the Mac Address of the specified type
  static MacAddress getMac(esp_mac_type_t type);
  static const std::unordered_map<esp_mac_type_t, MacAddress> &getMacMap() {
    return ESPmacMap;
  };

private:
  static std::once_flag macInitFlag;
  static void initMacs();
  friend void
  _preloadMacs(); // external function which triggers boot-time preload.
  // static MacAddress
  //     _mac[sizeof(esp_mac_type_str) / sizeof(esp_mac_type_str[0])];
  static void on_wifi_start(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data);

  static void ensureMacsInitialized();

#ifdef BT_ENABLED
//   void gap_event_handler(esp_gap_ble_cb_event_t event,
//                          esp_ble_gap_cb_param_t *param) {
//     if (event == ESP_GAP_BLE_ADV_START_COMPLETE_EVT &&
//         param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
//       MacAddress::get().refresh(ESP_MAC_BT); //TODO
// }

// };
#endif

  // TODO for bluetooth, it's more complicated. see
  //  esp_event_handler_register(BLUETOOTH_EVENT,
  //                             ESP_EVENT_BT_CONTROLLER_INIT_FINISH,
  //                             &MacTracker::on_bt_ready, NULL);
};

// esp_mac_type_t mactype;

// TODO add chip temperature sensing , section 34.3 of the ESP32C3 technical
// documentation

/// @brief lists the partition configuration of the NVS
void print_partitions();
void dumpSysInfo();
void print_dns_info();
void dump_ca_cert(const uint8_t *ca_crt_start, const uint8_t *ca_crt_end);
void DNSlookup(const char *nodeName);
} // namespace ED_SYSINFO