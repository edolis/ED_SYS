// #region StdManifest
/**
 * @file ED_sys.h
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

namespace ED_SYS {

    /**
     * @brief defines standard elements used to manage the device in the network.
     * It freezes the reference MAC, for instance, among the whole set of available MAC
     * and the standardized network and mqtt name
     *
     */
struct ESP_std {

    struct Firmware {
        static const char* prjName();
        static const char* version();
        static const char* date();
    };

    struct Device {
        static const char* stdMAC();  //standardized MAC name identifying this device
        static const char* netwName(); //standardized network name of the device
        static const char* mqttName(); //standardized MQTT client name for this device
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
    // Shared static storage
    static constexpr size_t MAC_STRLEN = 18;
    static constexpr size_t IP_STRLEN  = 16;

    static inline char _ESP_mqttID[MAC_STRLEN] = "";
    static inline char _ESP_NetwID[MAC_STRLEN] = "";
    static inline char _ESP_MAC[MAC_STRLEN]    = "";
    static inline char _ESP_IP[IP_STRLEN]      = "";
    static inline char _upTime[19]             = "";
    static inline char _time[26]               = "";

    static const inline esp_app_desc_t* app_desc = esp_app_get_description();
    static inline bool _ip_event_registered = false;
};

} // namespace ED_SYS
