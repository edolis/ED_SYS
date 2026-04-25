
#include <ED_esp_err.h>


const char* ED_err_to_name(esp_err_t err) {
    switch (err) {
        case ESP_ERR_SOCKET_CREATE_FAILED: return "ESP_ERR_SOCKET_CREATE_FAILED";
        case ESP_ERR_SOCKET_BIND_FAILED:   return "ESP_ERR_SOCKET_BIND_FAILED";
        case ESP_ERR_SOCKET_LISTEN_FAILED: return "ESP_ERR_SOCKET_LISTEN_FAILED";
        case ESP_ERR_SOCKET_ACCEPT_FAILED: return "ESP_ERR_SOCKET_ACCEPT_FAILED";
        default: return esp_err_to_name(err);  // fallback to ESP-IDF
    }
};