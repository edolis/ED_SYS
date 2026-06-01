/**
* @file I2C_w_recovery.cpp
* @brief OPT3001 test with interrupt on GPIO 6
used to verify the I2C bus recovery mechanism. try running and disconnecting a bus wire. it should recover.
 *
 * @author Emanuele Dolis (emanuele.dolis@gmail.com)
 * @version GIT_VERSION: v1.1.3-4-gf0e7061-dirty
 * @date 2026-06-01
 * @submodules-start
 *   ED_WIFI : v1.1.0-0-ga015030
 * @submodules-end
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ED_i2c.h"
#include "ED_OPT3001.h"

#define CONFIG_IDF_TARGET_ESP32C6
#include "ed_board.h"

#define I2C_FREQ 400000
#define OPT3001_ADDR 0x44

static const char *TAG = "main";

extern "C" void app_main() {
    // Suppress all logs from I2C bus and sensor to save stack
    esp_log_level_set("ed_i2c", ESP_LOG_ERROR);      // only errors
    esp_log_level_set("ED_OPT3001", ESP_LOG_ERROR);  // only errors

    I2CBus i2c(I2C_NUM_0, (gpio_num_t)ED_I2C_SDA, (gpio_num_t)ED_I2C_SCL, I2C_FREQ);
    ED_OPT3001::OPT3001 sensor(i2c, OPT3001_ADDR);

    // Init once (full config)
    ED_OPT3001::OPT3001::ConfigReg cfg{};
    cfg.mode_of_conversion = ED_OPT3001::OPT3001::CONTINUOUS_B;
    cfg.conversion_time    = ED_OPT3001::OPT3001::TIME_100MS;
    cfg.range_number_field = ED_OPT3001::OPT3001::RN_AUTO;
    sensor.configure(cfg);

    // Do NOT register any recovery callback – sensor retains settings across bus reset

    while (1) {
        float lux;
        if (sensor.readLux(lux) == ESP_OK) {
            ESP_LOGI(TAG, "Lux = %.2f", lux);
        } else {
            // The bus will automatically recover; no action needed
            ESP_LOGE(TAG, "Read failed – bus recovery in progress");
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}