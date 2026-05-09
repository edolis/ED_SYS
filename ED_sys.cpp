#include "ED_sys.h"
#include <esp_log.h>
#include <cstring>
#include <mutex>
#include <cstdio>               // for sscanf

namespace ED_SYS {

static const char *TAG = "ED_SYS";

// ===== Firmware =====
const char* ESP_std::Firmware::prjName() {
    return app_desc->project_name;
}

const char* ESP_std::Firmware::version() {
    return app_desc->version;
}

const char* ESP_std::Firmware::date() {
    return app_desc->date;
}

// --- Helper to parse the version string once ---
struct ParsedVersion {
    bool parsed = false;
    int major = 0, minor = 0, patch = 0, build = 0;
    char tag[32] = {0};
    char hash[16] = {0};       // includes leading 'g'
    bool dirty = false;
};

static ParsedVersion parseVersion() {
    ParsedVersion pv;
    const char* ver = ESP_std::Firmware::version();
    if (!ver || ver[0] == '\0') return pv;

    // Expected format: vMAJOR.MINOR.PATCH-BUILD-gHASH[-dirty]
    // Example: v0.0.1-4-gbb20eb4-dirty
    char dirty_str[6] = {0};
    int fields = sscanf(ver, "v%d.%d.%d-%d-%15s%5s",
                        &pv.major, &pv.minor, &pv.patch, &pv.build,
                        pv.hash, dirty_str);
    if (fields >= 5) {
        pv.parsed = true;
    } else if (fields >= 3) {
        // maybe version without build/hash? fallback
        pv.parsed = true;
    }
    if (pv.parsed) {
        if (strcmp(dirty_str, "-dirty") == 0)
            pv.dirty = true;
        else if (fields == 6 && strstr(ver, "-dirty"))
            pv.dirty = true;
    }

    // Extract tag from the beginning up to first '-'
    const char* first_dash = strchr(ver, '-');
    if (first_dash) {
        size_t tag_len = first_dash - ver;
        if (tag_len < sizeof(pv.tag)) {
            strncpy(pv.tag, ver, tag_len);
            pv.tag[tag_len] = '\0';
        }
    } else {
        // whole string is the tag
        strncpy(pv.tag, ver, sizeof(pv.tag) - 1);
    }
    return pv;
}

static const ParsedVersion& cachedVersion() {
    static ParsedVersion pv = parseVersion();
    return pv;
}

int  ESP_std::Firmware::majorVersion() { return cachedVersion().major; }
int  ESP_std::Firmware::minorVersion() { return cachedVersion().minor; }
int  ESP_std::Firmware::patchVersion() { return cachedVersion().patch; }
int  ESP_std::Firmware::buildNumber()  { return cachedVersion().build; }
const char* ESP_std::Firmware::tag()        { return cachedVersion().tag; }
const char* ESP_std::Firmware::shortHash()  { return cachedVersion().hash; }
bool ESP_std::Firmware::isDirty()           { return cachedVersion().dirty; }
const char* ESP_std::Firmware::fullHash() {
#ifdef FW_FULL_HASH
    return FW_FULL_HASH;
#else
    return "";
#endif
}

const char* ESP_std::Firmware::buildId() {
#ifdef FW_BUILD_ID
    return FW_BUILD_ID;
#else
    return "";
#endif
}

// ===== Device =====
const char* ESP_std::Device::stdMAC() {
    if (_ESP_MAC[0] == '\0') {
        strcpy(_ESP_MAC, ED_SYSINFO::ESP_MACstorage::getMac(esp_mac_type_t::ESP_MAC_BASE)
                             .toString(_ESP_MAC, sizeof(_ESP_MAC)));
    }
    return _ESP_MAC;
}

const char* ESP_std::Device::netwName() {
    if (_ESP_NetwID[0] == '\0') {
        auto mac = ED_SYSINFO::ESP_MACstorage::getMac(esp_mac_type_t::ESP_MAC_BASE);
        snprintf(_ESP_NetwID, sizeof(_ESP_NetwID), "ESP_%02X_%02X_%02X",
                 mac[3], mac[4], mac[5]);
    }
    return _ESP_NetwID;
}

const char* ESP_std::Device::mqttName() {
    if (_ESP_mqttID[0] == '\0') {
        auto mac = ED_SYSINFO::ESP_MACstorage::getMac(esp_mac_type_t::ESP_MAC_BASE);
        snprintf(_ESP_mqttID, sizeof(_ESP_mqttID), "ESP_%02X:%02X:%02X",
                 mac[3], mac[4], mac[5]);
    }
    return _ESP_mqttID;
}

// ===== Network =====
const char* ESP_std::Device::curIP() {
    {
        std::lock_guard<std::mutex> lock(s_ipMutex);
        if (_ESP_IP[0] != '\0') {
            return _ESP_IP;
        }
        get_current_ip(_ESP_IP, sizeof(_ESP_IP));
    }

    std::call_once(s_ipEventFlag, [](){
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                   &ESP_std::Device::on_ip_gotten, nullptr);
        ESP_LOGI(TAG, "IP event handler registered");
    });

    std::lock_guard<std::mutex> lock(s_ipMutex);
    return _ESP_IP;
}

void ESP_std::Device::on_ip_gotten(void*, esp_event_base_t, int32_t event_id, void* event_data) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
        auto* event = static_cast<ip_event_got_ip_t*>(event_data);
        char ip_buf[IP_STRLEN];
        esp_ip4addr_ntoa(&event->ip_info.ip, ip_buf, sizeof(ip_buf));

        {
            std::lock_guard<std::mutex> lock(s_ipMutex);
            strcpy(_ESP_IP, ip_buf);
        }
        ESP_LOGI(TAG, "IP updated to: %s", ip_buf);
    }
}

bool ESP_std::Device::get_current_ip(char* buf, size_t len) {
    esp_netif_ip_info_t ip_info;
    esp_netif_t* netif = esp_netif_get_default_netif();
    if (!netif) {
        ESP_LOGE(TAG, "Default network interface not found.");
        return false;
    }
    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        esp_ip4addr_ntoa(&ip_info.ip, buf, len);
        ESP_LOGI(TAG, "IP address retrieved: %s", buf);
        return true;
    }
    ESP_LOGW(TAG, "No IP address assigned yet.");
    memset(buf, 0, len);
    return false;
}

// ===== Runtime =====
const char* ESP_std::Runtime::uptime() {
    int64_t us_since_boot = esp_timer_get_time();
    time_t totalSeconds = us_since_boot / 1000000;
    uint16_t days = totalSeconds / 86400;
    uint8_t hours = (totalSeconds % 86400) / 3600;
    uint8_t minutes = (totalSeconds % 3600) / 60;
    uint8_t seconds = totalSeconds % 60;
    snprintf(_upTime, sizeof(_upTime), "%hu d %02u:%02u:%02u",
             days,
             (unsigned int)hours,
             (unsigned int)minutes,
             (unsigned int)seconds);
    return _upTime;
}

const char* ESP_std::Runtime::curStdTime() {
    ED_SNTP::TimeSync::getClockTime(ED_SNTP::ISOFORMAT::DATETIME_UTC_OFFSET,
                                    _time, sizeof(_time));
    ESP_LOGI(TAG, "Current time: %s", _time);
    return _time;
}



} // namespace ED_SYS