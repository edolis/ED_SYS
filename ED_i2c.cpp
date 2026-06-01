#include "ed_i2c.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/periph_ctrl.h"
#include "soc/periph_defs.h"
#include "rom/ets_sys.h"
#include <cstring>
#include <vector>

static const char *TAG = "ed_i2c";

#define I2C_MAX_RETRY_ATTEMPTS   -1
#define I2C_BASE_RETRY_DELAY_MS   200
#define I2C_MAX_RETRY_DELAY_MS    2000
#define PER_DEVICE_RETRY_LIMIT    3
#define POST_CLEAR_DELAY_MS       50

// ---------------------------------------------------------------------
// 9‑clock bus clear + STOP condition
// ---------------------------------------------------------------------
static void i2c_bus_clear(gpio_num_t sda, gpio_num_t scl) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << sda) | (1ULL << scl),
        .mode = GPIO_MODE_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_set_level(sda, 1);
    for (int i = 0; i < 9; i++) {
        gpio_set_level(scl, 0);
        esp_rom_delay_us(5);
        gpio_set_level(scl, 1);
        esp_rom_delay_us(5);
    }
    gpio_set_level(sda, 0);
    esp_rom_delay_us(5);
    gpio_set_level(scl, 1);
    esp_rom_delay_us(5);
    gpio_set_level(sda, 1);
    esp_rom_delay_us(5);
}

// ---------------------------------------------------------------------
I2CBus::I2CBus(i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t freq)
    : m_port(port), m_sda(sda), m_scl(scl), m_freq(freq), m_bus_handle(nullptr)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = port,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = { .enable_internal_pullup = true, .allow_pd = false },
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &m_bus_handle));
}

I2CBus::~I2CBus() {
    for (auto &p : m_devices) i2c_master_bus_rm_device(p.second);
    m_devices.clear();
    if (m_bus_handle) i2c_del_master_bus(m_bus_handle);
}

esp_err_t I2CBus::register_recovery_callback(uint8_t addr, std::function<esp_err_t(void)> cb) {
    if (!cb) return ESP_ERR_INVALID_ARG;
    m_recovery_callbacks[addr] = cb;
    m_failure_count[addr] = 0;
    return ESP_OK;
}

esp_err_t I2CBus::get_device(uint8_t addr, i2c_master_dev_handle_t *dev) {
    if (!m_bus_handle) return ESP_ERR_INVALID_STATE;
    auto it = m_devices.find(addr);
    if (it != m_devices.end()) {
        *dev = it->second;
        return ESP_OK;
    }
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = m_freq,
        .scl_wait_us = 0,
        .flags = { .disable_ack_check = false },
    };
    i2c_master_dev_handle_t new_dev;
    esp_err_t err = i2c_master_bus_add_device(m_bus_handle, &dev_cfg, &new_dev);
    if (err == ESP_OK) {
        m_devices[addr] = new_dev;
        *dev = new_dev;
    }
    return err;
}

// ---------------------------------------------------------------------
// Full hardware bus reset
// ---------------------------------------------------------------------
esp_err_t I2CBus::recover_bus() {
    // Save addresses
    std::vector<uint8_t> addrs;
    for (auto &p : m_devices) addrs.push_back(p.first);

    // Bus clear
    i2c_bus_clear(m_sda, m_scl);

    // Delete bus
    for (auto &p : m_devices) i2c_master_bus_rm_device(p.second);
    m_devices.clear();
    if (m_bus_handle) {
        i2c_del_master_bus(m_bus_handle);
        m_bus_handle = nullptr;
    }

    // Hardware peripheral reset
    periph_module_reset(PERIPH_I2C0_MODULE);
    esp_rom_delay_us(100);
    periph_module_enable(PERIPH_I2C0_MODULE);
    vTaskDelay(pdMS_TO_TICKS(POST_CLEAR_DELAY_MS));

    // Re‑create bus
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = m_port,
        .sda_io_num = m_sda,
        .scl_io_num = m_scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = { .enable_internal_pullup = true, .allow_pd = false },
    };
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &m_bus_handle);
    if (err != ESP_OK) return err;

    // Re‑add devices
    for (uint8_t a : addrs) {
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = a,
            .scl_speed_hz = m_freq,
            .scl_wait_us = 0,
            .flags = { .disable_ack_check = false },
        };
        i2c_master_dev_handle_t new_dev;
        if (i2c_master_bus_add_device(m_bus_handle, &dev_cfg, &new_dev) == ESP_OK)
            m_devices[a] = new_dev;
    }

    // Execute recovery callbacks
    for (auto &p : m_recovery_callbacks) p.second();
    for (auto &p : m_failure_count) p.second = 0;

    return ESP_OK;
}

// ---------------------------------------------------------------------
// Retry macro with per‑device soft recovery then full reset
// ---------------------------------------------------------------------
#define I2C_RETRY_WITH_RECOVERY(expr, addr) \
    do { \
        int _attempt = 0, _delay = I2C_BASE_RETRY_DELAY_MS; \
        esp_err_t _err; \
        while (1) { \
            i2c_master_dev_handle_t _dev; \
            _err = get_device(addr, &_dev); \
            if (_err == ESP_OK) { \
                _err = (expr); \
                if (_err == ESP_OK) { \
                    m_failure_count[addr] = 0; \
                    return ESP_OK; \
                } \
            } \
            int &fcnt = m_failure_count[addr]; \
            fcnt++; \
            bool per_ok = false; \
            auto it = m_recovery_callbacks.find(addr); \
            if (it != m_recovery_callbacks.end() && fcnt <= PER_DEVICE_RETRY_LIMIT) { \
                if (it->second() == ESP_OK) { per_ok = true; fcnt = 0; continue; } \
            } \
            if (!per_ok && fcnt > PER_DEVICE_RETRY_LIMIT) { \
                recover_bus(); \
                fcnt = 0; \
            } \
            _attempt++; \
            if (I2C_MAX_RETRY_ATTEMPTS > 0 && _attempt >= I2C_MAX_RETRY_ATTEMPTS) return _err; \
            vTaskDelay(pdMS_TO_TICKS(_delay)); \
            _delay = (_delay * 2 > I2C_MAX_RETRY_DELAY_MS) ? I2C_MAX_RETRY_DELAY_MS : _delay * 2; \
        } \
    } while(0)

esp_err_t I2CBus::write(uint8_t addr, const uint8_t *data, size_t len) {
    I2C_RETRY_WITH_RECOVERY(i2c_master_transmit(_dev, data, len, -1), addr);
}
esp_err_t I2CBus::read(uint8_t addr, uint8_t *data, size_t len) {
    I2C_RETRY_WITH_RECOVERY(i2c_master_receive(_dev, data, len, -1), addr);
}
esp_err_t I2CBus::write_then_read(uint8_t addr, const uint8_t *wdata, size_t wlen, uint8_t *rdata, size_t rlen) {
    I2C_RETRY_WITH_RECOVERY(i2c_master_transmit_receive(_dev, wdata, wlen, rdata, rlen, -1), addr);
}