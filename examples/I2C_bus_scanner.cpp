/**
* @file main.cpp
* @brief Standalone full-range I2C scanner (0x01 to 0x7F).
 *
 * @author Emanuele Dolis (emanuele.dolis@gmail.com)
 * @version GIT_VERSION: v1.1.3-5-g511346d-dirty
 * @date 2026-06-17
 * @submodules-start
 *   ED_WIFI : v1.1.0-1-g3b68ca4
 * @submodules-end
 */

#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c_master.h"

// ---- Pin definitions ----
#define I2C_SDA_GPIO         GPIO_NUM_22
#define I2C_SCL_GPIO         GPIO_NUM_23
#define I2C_PORT             I2C_NUM_0
#define I2C_FREQ_HZ          100000

static const char *TAG = "I2C_Scanner";

// ---- ANSI colour codes ----
#define COLOR_YELLOW  "\033[33m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RED     "\033[31m"
#define COLOR_RESET   "\033[0m"

// ---- Main ----
extern "C" void app_main(void) {
    // Suppress I2C driver logs
    esp_log_level_set("i2c.master", ESP_LOG_NONE);
    esp_log_level_set("i2c", ESP_LOG_NONE);

    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║              I2C FULL RANGE SCANNER (0x01-0x7F)         ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    printf("SDA=GPIO%d, SCL=GPIO%d @ %d Hz\n\n", I2C_SDA_GPIO, I2C_SCL_GPIO, I2C_FREQ_HZ);

    // ---- 1. Configure I2C bus ----
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        },
    };

    i2c_master_bus_handle_t bus_handle;
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(err));
        while (1) vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // ---- 2. Scan full range 0x01 to 0x7F ----
    printf("     ");
    for (int col = 0; col < 16; col++) {
        printf(" %02X ", col);
    }
    printf("\n");
    printf("     ");
    for (int col = 0; col < 16; col++) {
        printf("────");
    }
    printf("\n");

    int found_count = 0;
    bool found_as7341 = false;

    // Start from 0x01 (skip 0x00 which is general call)
    for (uint8_t addr = 0x01; addr <= 0x7F; addr++) {
        // Print new row every 16 addresses
        if ((addr & 0x0F) == 0x00) {
            printf("0x%02X: ", addr);
        }

        // Probe with 50ms timeout
        err = i2c_master_probe(bus_handle, addr, 50);

        if (err == ESP_OK) {
            printf(COLOR_GREEN " OK " COLOR_RESET);
            found_count++;
            if (addr == 0x39) {
                found_as7341 = true;
            }
        } else {
            printf(COLOR_YELLOW " -- " COLOR_RESET);
        }

        // New line after 16 addresses
        if ((addr & 0x0F) == 0x0F) {
            printf("\n");
        }
    }

    // ---- 3. Summary ----
    printf("\n\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║                      SCAN RESULTS                      ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("Total devices found: " COLOR_GREEN "%d" COLOR_RESET "\n", found_count);

    // Check specifically for 0x39 (AS7341)
    printf("\nChecking for AS7341 at 0x39... ");
    if (found_as7341) {
        printf(COLOR_GREEN "✓ FOUND" COLOR_RESET "\n");
    } else {
        printf(COLOR_RED "✗ NOT FOUND" COLOR_RESET "\n");
        printf("\nTroubleshooting:\n");
        printf("  □ Check power: 3.3V for module, 1.8V for bare chip\n");
        printf("  □ Check wiring: SDA→GPIO22, SCL→GPIO23, GND→GND\n");
        printf("  □ Check pull-up resistors: 4.7kΩ on SDA and SCL\n");
        printf("  □ Check sensor orientation/pinout\n");
    }

    // ---- 4. List all found addresses ----
    if (found_count > 0) {
        printf("\nFound at addresses: ");
        bool first = true;
        for (uint8_t addr = 0x01; addr <= 0x7F; addr++) {
            err = i2c_master_probe(bus_handle, addr, 20);
            if (err == ESP_OK) {
                if (!first) printf(", ");
                printf("0x%02X", addr);
                first = false;
            }
        }
        printf("\n");
    }

    // ---- 5. Clean up ----
    i2c_del_master_bus(bus_handle);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}