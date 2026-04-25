#pragma once


/***************************
 *
CUSTOM ERRORS for ED series projects

 */
#include "esp_err.h"

#define ESP_ERR_CUSTOM_BASE         0x50000


#ifndef ERROR_CODES_H
#define ERROR_CODES_H

#define ED_ERR_CORE_BASE        0x50000  // Core system errors: init, memory, threading, internal logic
#define ED_ERR_CORE_MAX         0x50FFF  // End of core system error range

#define ED_ERR_NET_BASE         0x51000  // Networking errors: TCP/UDP, sockets, DNS, TLS/SSL, HTTP
#define ED_ERR_NET_MAX          0x51FFF  // End of networking error range

#define ED_ERR_STORAGE_BASE     0x52000  // Storage/filesystem errors: file I/O, mount, permissions, corruption
#define ED_ERR_STORAGE_MAX      0x52FFF  // End of storage error range

#define ED_ERR_UI_BASE          0x53000  // UI/display errors: rendering, input handling, screen faults
#define ED_ERR_UI_MAX           0x53FFF  // End of UI error range

#define ED_ERR_SENSOR_BASE      0x54000  // Sensor/hardware errors: read failures, calibration, device faults
#define ED_ERR_SENSOR_MAX       0x54FFF  // End of sensor error range

#define ED_ERR_SECURITY_BASE    0x55000  // Security/authentication errors: encryption, access control, auth
#define ED_ERR_SECURITY_MAX     0x55FFF  // End of security error range

#define ED_ERR_DB_BASE          0x56000  // Database/persistence errors: SQL, connection, data integrity
#define ED_ERR_DB_MAX           0x56FFF  // End of database error range

#define ED_ERR_CLOUD_BASE       0x57000  // Cloud/API errors: REST, tokens, rate limits, connectivity
#define ED_ERR_CLOUD_MAX        0x57FFF  // End of cloud error range

#define ED_ERR_BT_BASE          0x58000  // Bluetooth/wireless errors: pairing, protocol, connection issues
#define ED_ERR_BT_MAX           0x58FFF  // End of Bluetooth error range

#define ED_ERR_POWER_BASE       0x59000  // Power/battery errors: voltage, charging, battery health
#define ED_ERR_POWER_MAX        0x59FFF  // End of power error range

#define ED_ERR_MEDIA_BASE       0x5A000  // Audio/media errors: playback, codecs, streaming
#define ED_ERR_MEDIA_MAX        0x5AFFF  // End of media error range

#define ED_ERR_LOCALE_BASE      0x5B000  // Localization/internationalization errors: language, formatting
#define ED_ERR_LOCALE_MAX       0x5BFFF  // End of locale error range

#define ED_ERR_CONFIG_BASE      0x5C000  // Configuration/settings errors: invalid config, missing keys
#define ED_ERR_CONFIG_MAX       0x5CFFF  // End of config error range

#define ED_ERR_DIAG_BASE        0x5D000  // Diagnostics/logging errors: telemetry, debug, log failures
#define ED_ERR_DIAG_MAX         0x5DFFF  // End of diagnostics error range

#define ED_ERR_CUSTOM_BASE      0x5E000  // Custom/reserved errors: app-specific or vendor extensions
#define ED_ERR_CUSTOM_MAX       0x5EFFF  // End of custom error range

#define ED_ERR_LEGACY_BASE      0x5F000  // Legacy/compatibility errors: deprecated modules, old APIs
#define ED_ERR_LEGACY_MAX       0x5FFFF  // End of legacy error range

#endif // ERROR_CODES_H

// Specific error codes
#define ESP_ERR_SOCKET_CREATE_FAILED (ED_ERR_NET_BASE + 1) //failed to create a new socket
#define ESP_ERR_SOCKET_BIND_FAILED   (ED_ERR_NET_BASE + 2)
#define ESP_ERR_SOCKET_LISTEN_FAILED (ED_ERR_NET_BASE + 3)
#define ESP_ERR_SOCKET_ACCEPT_FAILED (ED_ERR_NET_BASE + 4)

// custom_errors.c

const char* ED_err_to_name(esp_err_t err) ;
