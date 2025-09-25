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
#pragma once

#include "ED_json.h"
#include "esp_app_desc.h"
#include "esp_mac.h"
#include <esp_event.h>
#include <mutex>
#include <string>
#include <unordered_map>

static constexpr esp_mac_type_t ESP_MAC_NOTSET =
    static_cast<esp_mac_type_t>(-1);

namespace ED_SYSINFO {


// ==================== fwInfo ====================
class fwInfo {
public:
    struct VersionInfo {
        int major = 0;
        int minor = 0;
        int patch = 0;
        int commitsAhead = 0;
        bool isDirty = false;
        bool isValidGIT = false;
        std::string full;
        std::string GITcommitID;
        bool isEmpty();
    };

    static const char* PrjName();
    static const char* AppCompileDate();
    static const VersionInfo& AppVers();
    static const char* AppCompileTime();
    static const char* AppELFSha256();
    static const char* AppESPIDF();

private:
    static const char* _AppVersStr();
    static VersionInfo& parseVersion(const char* versionStr);
    static VersionInfo* _Vinfo;
    static const esp_app_desc_t* getDesc();
};

// ==================== MacAddress ====================
class MacAddress {
public:
    MacAddress();
    MacAddress(const uint8_t mac[6]);
    const uint8_t& operator[](size_t index) const;
    const uint8_t* get() const;
    uint8_t* set(const uint8_t mac[6]);
    char* toString(char* buffer, size_t buffer_size, char separator = ':') const;
    bool isValid() const;

private:
    bool isZeroMac(const uint8_t mac[6]) const;
    static inline const uint8_t zeroMac[6] = {0};
    uint8_t _mac_addr[6];
    bool _isValid = false;
};

// ==================== ESP_MACstorage ====================
class ESP_MACstorage {
public:
    static void initListener();
    static MacAddress getMac(esp_mac_type_t type);
    static const std::unordered_map<esp_mac_type_t, MacAddress>& getMacMap();

private:
    static std::mutex macMapMutex;
    static std::unordered_map<esp_mac_type_t, MacAddress> ESPmacMap;
    static std::once_flag macInitFlag;
    static void initMacs();
    static void on_wifi_start(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data);
    static void ensureMacsInitialized();
};

// ==================== Free functions ====================
void print_partitions();
void dumpSysInfo();
void print_dns_info();
void dump_ca_cert(const uint8_t* ca_crt_start, const uint8_t* ca_crt_end);
void DNSlookup(const char* nodeName);

} // namespace ED_SYSINFO
