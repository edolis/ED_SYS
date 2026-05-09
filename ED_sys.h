#pragma once

#include "ED_SNTP_time.h"
#include "ED_sysInfo.h"
#include "esp_netif.h"
#include "esp_wifi_types_generic.h"
#include "esp_event.h"
#include "esp_app_desc.h"
#include <esp_timer.h>
#include <cstring>
#include <mutex>

namespace ED_SYS {

struct ESP_std {

    struct Firmware {
        // Original basic info
        static const char* prjName();
        static const char* version();          // full string, e.g. v0.0.1-4-gbb20eb4-dirty
        static const char* date();

        // Parsed components from version()
        static int  majorVersion();
        static int  minorVersion();
        static int  patchVersion();
        static int  buildNumber();             // commits since tag
        static const char* tag();              // e.g. "v0.0.1"
        static const char* shortHash();        // e.g. "gbb20eb4"
        static bool isDirty();                 // true if version ends with "-dirty"

        // Additional build‑time constants (from version.h)
        static const char* fullHash();         // 40‑char hex string
        static const char* buildId();          // e.g. "P20260509-140435-179518"
    };

    struct Device {
        static const char* stdMAC();
        static const char* netwName();
        static const char* mqttName();
        static const char* curIP();
    private:
        static void on_ip_gotten(void* handler_arg, esp_event_base_t event_base,
                                 int32_t event_id, void* event_data);
        static bool get_current_ip(char* ip_address_str, size_t buffer_len);
    };

    struct Runtime {
        static const char* uptime();
        static const char* curStdTime();
    };

private:
    static constexpr size_t MAC_STRLEN = 18;
    static constexpr size_t IP_STRLEN  = 16;

    static inline char _ESP_mqttID[MAC_STRLEN] = "";
    static inline char _ESP_NetwID[MAC_STRLEN] = "";
    static inline char _ESP_MAC[MAC_STRLEN]    = "";
    static inline char _upTime[19]             = "";
    static inline char _time[26]               = "";

    static const inline esp_app_desc_t* app_desc = esp_app_get_description();

    static inline std::mutex   s_ipMutex;
    static inline char         _ESP_IP[IP_STRLEN] = "";
    static inline std::once_flag s_ipEventFlag;
};

} // namespace ED_SYS