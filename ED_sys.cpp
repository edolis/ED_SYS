
    #include "ED_sys.h"
    #include <esp_log.h>

    using namespace ED_SYSINFO;

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

// ===== Device =====
const char* ESP_std::Device::stdMAC() {
    if (_ESP_MAC[0] == '\0') {
        strcpy(_ESP_MAC, ESP_MACstorage::getMac(esp_mac_type_t::ESP_MAC_BASE)
                             .toString(_ESP_MAC, sizeof(_ESP_MAC)));
    }
    return _ESP_MAC;
}

const char* ESP_std::Device::netwName() {
    if (_ESP_NetwID[0] == '\0') {
        snprintf(_ESP_NetwID, sizeof(_ESP_NetwID), "ESP_%02X_%02X_%02X",
                 ESP_MACstorage::getMac(esp_mac_type_t::ESP_MAC_BASE)[3],
                 ESP_MACstorage::getMac(esp_mac_type_t::ESP_MAC_BASE)[4],
                 ESP_MACstorage::getMac(esp_mac_type_t::ESP_MAC_BASE)[5]);
    }
    return _ESP_NetwID;
}

const char* ESP_std::Device::mqttName() {
    if (_ESP_mqttID[0] == '\0') {
        snprintf(_ESP_mqttID, sizeof(_ESP_mqttID), "ESP_%02X:%02X:%02X",
                 ESP_MACstorage::getMac(esp_mac_type_t::ESP_MAC_BASE)[3],
                 ESP_MACstorage::getMac(esp_mac_type_t::ESP_MAC_BASE)[4],
                 ESP_MACstorage::getMac(esp_mac_type_t::ESP_MAC_BASE)[5]);
    }
    return _ESP_mqttID;
}

// ===== Network =====
const char* ESP_std::Device::curIP() {
    if (_ESP_IP[0] == '\0') {
        get_current_ip(_ESP_IP, sizeof(_ESP_IP));
        if (!_ip_event_registered) {
            esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                       &ESP_std::Device::on_ip_gotten, nullptr);
            _ip_event_registered = true;
        }
    }
    return _ESP_IP;
}

void ESP_std::Device::on_ip_gotten(void*, esp_event_base_t, int32_t event_id, void* event_data) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
        auto* event = static_cast<ip_event_got_ip_t*>(event_data);
        esp_ip4addr_ntoa(&event->ip_info.ip, _ESP_IP, sizeof(_ESP_IP));
        ESP_LOGI("STORAGE", "IP updated to: %s", _ESP_IP);
    }
}

bool ESP_std::Device::get_current_ip(char* buf, size_t len) {
    esp_netif_ip_info_t ip_info;
    esp_netif_t* netif = esp_netif_get_default_netif();
    if (!netif) {
        ESP_LOGE("GET_IP", "Default network interface not found.");
        return false;
    }
    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        esp_ip4addr_ntoa(&ip_info.ip, buf, len);
        ESP_LOGI("GET_IP", "IP address retrieved: %s", buf);
        return true;
    }
    ESP_LOGW("GET_IP", "No IP address assigned yet.");
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
    snprintf(_upTime, sizeof(_upTime), "%2ud %02u:%02u:%02u",
             days, hours, minutes, seconds);
    return _upTime;
}

const char* ESP_std::Runtime::curStdTime() {
  ED_SNTP::TimeSync::getClockTime(ED_SNTP::ISOFORMAT::DATETIME_UTC_OFFSET,_time,sizeof(_time));
  ESP_LOGI(TAG,"Step_curStdTime %s", _time);
return _time;
}

} // namespace ED_SYS
