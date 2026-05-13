/**
 * ed_board.h – ED_SYS
 *
 * Centralized board pin definitions for ESP32 variants used across projects.
 * Select your board by defining one of the BOARD_VARIANT_xxx macros *before*
 * including this header.  If no variant is defined, the header falls back to
 * chip‑target defaults that match the most common dev‑kit for that chip.
 *
 * Supported boards (define ONE before #include):
 *   BOARD_VARIANT_ESP32S3_SUPERMINI     Tenstar Robot ESP32‑S3 SuperMini (black PCB)
 *   BOARD_VARIANT_ESP32S3_ZERO          Waveshare ESP32‑S3‑Zero (also “Zero‑M”)
 *   BOARD_VARIANT_ESP32C6_ZERO          Waveshare ESP32‑C6‑Zero
 *   BOARD_VARIANT_ESP32C3_SUPERMINI     ESP32‑C3 SuperMini (various mfrs, blue PCB)
 *
 * Chip‑target fallback (used when no BOARD_VARIANT is defined):
 *   CONFIG_IDF_TARGET_ESP32S3    → Espressif S3‑DevKitC‑1 defaults
 *   CONFIG_IDF_TARGET_ESP32C3    → Espressif C3‑DevKitM‑1 defaults
 *   CONFIG_IDF_TARGET_ESP32C6    → Espressif C6‑DevKitC‑1 defaults
 *   CONFIG_IDF_TARGET_ESP32      → Espressif ESP32‑DevKitC defaults
 *
 * Board features sorted by available GPIO pins (most → fewest):
 *   ESP32‑S3 SuperMini – ~38 exposed GPIOs
 *   ESP32‑S3‑Zero      – 24 exposed GPIOs
 *   ESP32‑C6‑Zero      – 22 exposed GPIOs
 *   ESP32‑C3 SuperMini – 11 programmable GPIOs
 *
 * Detailed descriptions are at the bottom of this file.
 */

#ifndef ED_BOARD_H
#define ED_BOARD_H

#include "hal/gpio_types.h"
#include "sdkconfig.h"

/* ------------------------------------------------------------------ */
/*  User‑definable overrides (set before #include)                    */
/* ------------------------------------------------------------------ */
#if defined(DOXYGEN)
    #define BOARD_VARIANT_ESP32S3_ZERO
    #define ED_ONBOARD_LED   GPIO_NUM_48
    #define ED_I2C_SDA       GPIO_NUM_8
    #define ED_I2C_SCL       GPIO_NUM_9
    #define ED_BOOT_BUTTON   GPIO_NUM_0
    #define ED_SPI_SCK       GPIO_NUM_12
    #define ED_SPI_MOSI      GPIO_NUM_11
    #define ED_SPI_MISO      GPIO_NUM_13
    #define ED_SPI_CS        GPIO_NUM_10
#endif

/* ------------------------------------------------------------------ */
/*  1.  ON‑BOARD LED                                                   */
/* ------------------------------------------------------------------ */
#if !defined(ED_ONBOARD_LED)
    #if defined(BOARD_VARIANT_ESP32S3_SUPERMINI)
        #define ED_ONBOARD_LED  GPIO_NUM_48
    #elif defined(BOARD_VARIANT_ESP32S3_ZERO)
        #define ED_ONBOARD_LED  GPIO_NUM_21
    #elif defined(BOARD_VARIANT_ESP32C6_ZERO)
        #define ED_ONBOARD_LED  GPIO_NUM_8
    #elif defined(BOARD_VARIANT_ESP32C3_SUPERMINI)
        #define ED_ONBOARD_LED  GPIO_NUM_8
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
        #define ED_ONBOARD_LED  GPIO_NUM_48
    #elif defined(CONFIG_IDF_TARGET_ESP32C6)
        #define ED_ONBOARD_LED  GPIO_NUM_8
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
        #define ED_ONBOARD_LED  GPIO_NUM_8
    #elif defined(CONFIG_IDF_TARGET_ESP32)
        #define ED_ONBOARD_LED  GPIO_NUM_2
    #else
        #warning "ed_board: Unknown chip target – LED defaulted to GPIO 2"
        #define ED_ONBOARD_LED  GPIO_NUM_2
    #endif
#endif

/* ------------------------------------------------------------------ */
/*  2.  I²C pins                                                        */
/* ------------------------------------------------------------------ */
#if !defined(ED_I2C_SDA)
    #if defined(BOARD_VARIANT_ESP32S3_SUPERMINI) || \
        defined(BOARD_VARIANT_ESP32S3_ZERO)
        #define ED_I2C_SDA  GPIO_NUM_8
    #elif defined(BOARD_VARIANT_ESP32C6_ZERO)
        #define ED_I2C_SDA  GPIO_NUM_22
    #elif defined(BOARD_VARIANT_ESP32C3_SUPERMINI)
        #define ED_I2C_SDA  GPIO_NUM_8
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
        #define ED_I2C_SDA  GPIO_NUM_8
    #elif defined(CONFIG_IDF_TARGET_ESP32C6)
        #define ED_I2C_SDA  GPIO_NUM_22
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
        #define ED_I2C_SDA  GPIO_NUM_8
    #elif defined(CONFIG_IDF_TARGET_ESP32)
        #define ED_I2C_SDA  GPIO_NUM_21
    #else
        #define ED_I2C_SDA  GPIO_NUM_21
    #endif
#endif

#if !defined(ED_I2C_SCL)
    #if defined(BOARD_VARIANT_ESP32S3_SUPERMINI) || \
        defined(BOARD_VARIANT_ESP32S3_ZERO)
        #define ED_I2C_SCL  GPIO_NUM_9
    #elif defined(BOARD_VARIANT_ESP32C6_ZERO)
        #define ED_I2C_SCL  GPIO_NUM_23
    #elif defined(BOARD_VARIANT_ESP32C3_SUPERMINI)
        #define ED_I2C_SCL  GPIO_NUM_9
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
        #define ED_I2C_SCL  GPIO_NUM_9
    #elif defined(CONFIG_IDF_TARGET_ESP32C6)
        #define ED_I2C_SCL  GPIO_NUM_23
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
        #define ED_I2C_SCL  GPIO_NUM_9
    #elif defined(CONFIG_IDF_TARGET_ESP32)
        #define ED_I2C_SCL  GPIO_NUM_22
    #else
        #define ED_I2C_SCL  GPIO_NUM_22
    #endif
#endif

/* ------------------------------------------------------------------ */
/*  3.  BOOT button                                                     */
/* ------------------------------------------------------------------ */
#if !defined(ED_BOOT_BUTTON)
    #if defined(BOARD_VARIANT_ESP32S3_SUPERMINI)
        #define ED_BOOT_BUTTON  GPIO_NUM_0
    #elif defined(BOARD_VARIANT_ESP32S3_ZERO)
        #define ED_BOOT_BUTTON  GPIO_NUM_0
    #elif defined(BOARD_VARIANT_ESP32C6_ZERO)
        #define ED_BOOT_BUTTON  GPIO_NUM_9
    #elif defined(BOARD_VARIANT_ESP32C3_SUPERMINI)
        #define ED_BOOT_BUTTON  GPIO_NUM_9
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
        #define ED_BOOT_BUTTON  GPIO_NUM_0
    #elif defined(CONFIG_IDF_TARGET_ESP32C6)
        #define ED_BOOT_BUTTON  GPIO_NUM_9
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
        #define ED_BOOT_BUTTON  GPIO_NUM_9
    #elif defined(CONFIG_IDF_TARGET_ESP32)
        #define ED_BOOT_BUTTON  GPIO_NUM_0
    #else
        #define ED_BOOT_BUTTON  GPIO_NUM_0
    #endif
#endif

/* ------------------------------------------------------------------ */
/*  4.  SPI pins (default VSPI / FSPI)                                  */
/* ------------------------------------------------------------------ */
#if !defined(ED_SPI_SCK)
    #if defined(BOARD_VARIANT_ESP32S3_SUPERMINI)
        #define ED_SPI_SCK   GPIO_NUM_12
    #elif defined(BOARD_VARIANT_ESP32S3_ZERO)
        #define ED_SPI_SCK   GPIO_NUM_12
    #elif defined(BOARD_VARIANT_ESP32C6_ZERO)
        #define ED_SPI_SCK   GPIO_NUM_20
    #elif defined(BOARD_VARIANT_ESP32C3_SUPERMINI)
        #define ED_SPI_SCK   GPIO_NUM_10
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
        #define ED_SPI_SCK   GPIO_NUM_12
    #elif defined(CONFIG_IDF_TARGET_ESP32C6)
        #define ED_SPI_SCK   GPIO_NUM_20
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
        #define ED_SPI_SCK   GPIO_NUM_10
    #elif defined(CONFIG_IDF_TARGET_ESP32)
        #define ED_SPI_SCK   GPIO_NUM_18
    #else
        #define ED_SPI_SCK   GPIO_NUM_18
    #endif
#endif

#if !defined(ED_SPI_MOSI)
    #if defined(BOARD_VARIANT_ESP32S3_SUPERMINI)
        #define ED_SPI_MOSI  GPIO_NUM_11
    #elif defined(BOARD_VARIANT_ESP32S3_ZERO)
        #define ED_SPI_MOSI  GPIO_NUM_11
    #elif defined(BOARD_VARIANT_ESP32C6_ZERO)
        #define ED_SPI_MOSI  GPIO_NUM_19
    #elif defined(BOARD_VARIANT_ESP32C3_SUPERMINI)
        #define ED_SPI_MOSI  GPIO_NUM_7
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
        #define ED_SPI_MOSI  GPIO_NUM_11
    #elif defined(CONFIG_IDF_TARGET_ESP32C6)
        #define ED_SPI_MOSI  GPIO_NUM_19
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
        #define ED_SPI_MOSI  GPIO_NUM_7
    #elif defined(CONFIG_IDF_TARGET_ESP32)
        #define ED_SPI_MOSI  GPIO_NUM_23
    #else
        #define ED_SPI_MOSI  GPIO_NUM_23
    #endif
#endif

#if !defined(ED_SPI_MISO)
    #if defined(BOARD_VARIANT_ESP32S3_SUPERMINI)
        #define ED_SPI_MISO  GPIO_NUM_13
    #elif defined(BOARD_VARIANT_ESP32S3_ZERO)
        #define ED_SPI_MISO  GPIO_NUM_13
    #elif defined(BOARD_VARIANT_ESP32C6_ZERO)
        #define ED_SPI_MISO  GPIO_NUM_18
    #elif defined(BOARD_VARIANT_ESP32C3_SUPERMINI)
        #define ED_SPI_MISO  GPIO_NUM_2
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
        #define ED_SPI_MISO  GPIO_NUM_13
    #elif defined(CONFIG_IDF_TARGET_ESP32C6)
        #define ED_SPI_MISO  GPIO_NUM_18
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
        #define ED_SPI_MISO  GPIO_NUM_2
    #elif defined(CONFIG_IDF_TARGET_ESP32)
        #define ED_SPI_MISO  GPIO_NUM_19
    #else
        #define ED_SPI_MISO  GPIO_NUM_19
    #endif
#endif

#if !defined(ED_SPI_CS)
    #if defined(BOARD_VARIANT_ESP32S3_SUPERMINI)
        #define ED_SPI_CS    GPIO_NUM_10
    #elif defined(BOARD_VARIANT_ESP32S3_ZERO)
        #define ED_SPI_CS    GPIO_NUM_10
    #elif defined(BOARD_VARIANT_ESP32C6_ZERO)
        #define ED_SPI_CS    GPIO_NUM_21
    #elif defined(BOARD_VARIANT_ESP32C3_SUPERMINI)
        #define ED_SPI_CS    GPIO_NUM_9
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
        #define ED_SPI_CS    GPIO_NUM_10
    #elif defined(CONFIG_IDF_TARGET_ESP32C6)
        #define ED_SPI_CS    GPIO_NUM_21
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
        #define ED_SPI_CS    GPIO_NUM_9
    #elif defined(CONFIG_IDF_TARGET_ESP32)
        #define ED_SPI_CS    GPIO_NUM_5
    #else
        #define ED_SPI_CS    GPIO_NUM_5
    #endif
#endif

/* ------------------------------------------------------------------ */
/*  Detailed board feature summaries (sorted by available GPIOs)       */
/* ------------------------------------------------------------------ */

/*
 * 1. BOARD_VARIANT_ESP32S3_SUPERMINI
 *    Tenstar Robot ESP32‑S3 SuperMini (black PCB)
 *    • Reference: https://www.espboards.dev/esp32/esp32-s3-super-mini/
 *    • Chip:        ESP32‑S3 (Xtensa LX7 dual‑core, up to 240 MHz)
 *    • Memory:      512 KB SRAM, 384 KB ROM, 4 MB Flash, 2 MB PSRAM
 *    • Available GPIOs:  ~38 (exact number depends on revision;
 *                       all except the Octal PSRAM pins are exposed)
 *    • Wireless:    Wi‑Fi 4 (802.11 b/g/n), Bluetooth 5.0 (LE)
 *                   **No native Zigbee** – external 802.15.4 radio
 *                   (e.g., ESP32‑H2) needed for Zigbee/Thread
 *    • LED:         WS2812 RGB (GPIO48) – colour order GRB
 *    • USB:         USB‑C (integrated USB‑to‑UART + native USB‑JTAG)
 *    • Buttons:     BOOT (GPIO0), RESET
 *    • I²C default: SDA = GPIO8, SCL = GPIO9
 *    • SPI default: SCK = GPIO12, MOSI = GPIO11, MISO = GPIO13, CS = GPIO10
 *    • Other:       LiPo battery charger circuit on‑board (for Li-ion/Li‑Po,
 *                   NOT LiFePO4). Full charge voltage is 4.2V.
 *
 * 2. BOARD_VARIANT_ESP32S3_ZERO
 *    Waveshare ESP32‑S3‑Zero (also Zero‑M)
 *    • Reference: https://www.espboards.dev/esp32/esp32-s3-zero/
 *    • Chip:        ESP32‑S3FH4R2 (Xtensa LX7 dual‑core, up to 240 MHz)
 *    • Memory:      512 KB SRAM, 384 KB ROM, 4 MB Flash, 2 MB PSRAM
 *                   (8 MB Flash / 8 MB PSRAM variants also available)
 *    • Available GPIOs:  24 (GPIO33‑37 reserved for Octal PSRAM)
 *    • Wireless:    Wi‑Fi 4 (802.11 b/g/n), Bluetooth 5.0 (LE)
 *                   **No native Zigbee** – external 802.15.4 radio needed
 *    • LED:         WS2812 RGB (GPIO21)
 *    • USB:         Native USB‑C (no external UART bridge); hold BOOT to flash
 *    • UART default: TX = GPIO43, RX = GPIO44
 *    • Buttons:     BOOT (GPIO0), RESET
 *    • I²C default: SDA = GPIO8, SCL = GPIO9
 *    • SPI default: SCK = GPIO12, MOSI = GPIO11, MISO = GPIO13, CS = GPIO10
 *    • Form‑factor: Stamp‑hole / castellated, 18 × 23.5 mm
 *    • Other:       HW crypto (AES, RSA, SHA, HMAC), Secure Boot compatible
 *
 * 3. BOARD_VARIANT_ESP32C6_ZERO
 *    Waveshare ESP32‑C6‑Zero
 *    • Reference: https://www.espboards.dev/esp32/esp32-c6-zero-mini/
 *    • Chip:        ESP32‑C6FH8 (RISC‑V single‑core, up to 160 MHz)
 *                   (4 MB Flash variant also available)
 *    • Memory:      512 KB HP SRAM, 16 KB LP SRAM, 320 KB ROM
 *                   4 MB Flash (or 8 MB on FH8 variant)
 *    • Available GPIOs:  22 (on stamp‑hole pads)
 *    • Wireless:    Wi‑Fi 6 (802.11 ax), Bluetooth 5 (LE)
 *                   **Native IEEE 802.15.4 – Zigbee 3.0 / Thread**
 *                   This is the only board with built‑in Zigbee support.
 *    • LED:         WS2812 RGB (GPIO8)
 *    • USB:         USB‑C (native USB‑to‑UART + JTAG)
 *    • Buttons:     BOOT (GPIO9, active low), RESET
 *    • I²C default: SDA = GPIO22, SCL = GPIO23
 *    • SPI default: SCK = GPIO20, MOSI = GPIO19, MISO = GPIO18, CS = GPIO21
 *    • Form‑factor: Stamp‑hole / castellated, similar size to S3‑Zero
 *
 * 4. BOARD_VARIANT_ESP32C3_SUPERMINI
 *    ESP32‑C3 SuperMini (various manufacturers, blue PCB)
 *    • Reference: https://www.espboards.dev/esp32/esp32-c3-super-mini/
 *    • Chip:        ESP32‑C3 (RISC‑V single‑core, up to 160 MHz)
 *    • Memory:      400 KB SRAM, 384 KB ROM, 4 MB Flash
 *    • Available GPIOs:  11 (programmable: GPIO0‑10, 20, 21)
 *    • Wireless:    Wi‑Fi 4 (802.11 b/g/n), Bluetooth 5.0 (LE)
 *                   **No native Zigbee** – external 802.15.4 radio needed
 *    • LED:         Blue LED on GPIO8 – **INVERTED** (ON when pin LOW)
 *    • USB:         USB‑C (built‑in USB‑to‑UART + JTAG)
 *    • Buttons:     BOOT (GPIO9, also used as SCL), RESET
 *    • I²C default: SDA = GPIO8, SCL = GPIO9
 *                   (GPIO8 & 9 are strapping pins – test boot behaviour)
 *    • SPI default: SCK = GPIO10, MOSI = GPIO7, MISO = GPIO2, CS = GPIO9
 *                   (CS pin shared with BOOT/SCL – use with caution)
 *    • Form‑factor: ~18 × 22 mm (tiny)
 */

#endif /* ED_BOARD_H */