#include "ed_i2c.h"
#include "esp_log.h"
#include "esp_check.h"
#include <cstring>

static const char *TAG = "ed_i2c";

bool I2CBus::s_installed[I2C_NUM_MAX] = { false };

I2CBus::I2CBus(i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t freq)
    : m_bus_handle(nullptr), m_freq(freq)
{
    if (!s_installed[port]) {
        i2c_master_bus_config_t bus_cfg = {
            .i2c_port = port,
            .sda_io_num = sda,
            .scl_io_num = scl,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags = {
                .enable_internal_pullup = true,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &m_bus_handle));
        s_installed[port] = true;
        ESP_LOGI(TAG, "I2C master bus on port %d initialized at %lu Hz", port, freq);
    } else {
        ESP_LOGW(TAG, "Bus %d already installed; new I2CBus instance will not own it", port);
        m_bus_handle = nullptr;
    }
}

I2CBus::~I2CBus()
{
    for (auto &pair : m_devices) {
        i2c_master_bus_rm_device(pair.second);
    }
    m_devices.clear();
    if (m_bus_handle != nullptr) {
        i2c_del_master_bus(m_bus_handle);
    }
}

esp_err_t I2CBus::get_device(uint8_t dev_addr, i2c_master_dev_handle_t *dev_handle)
{
    if (m_bus_handle == nullptr) {
        ESP_LOGE(TAG, "Bus not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    auto it = m_devices.find(dev_addr);
    if (it != m_devices.end()) {
        *dev_handle = it->second;
        return ESP_OK;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = dev_addr,
        .scl_speed_hz = m_freq,
        .flags = {
            .disable_ack_check = false,
        },
    };
    i2c_master_dev_handle_t new_dev;
    esp_err_t err = i2c_master_bus_add_device(m_bus_handle, &dev_cfg, &new_dev);
    if (err == ESP_OK) {
        m_devices[dev_addr] = new_dev;
        *dev_handle = new_dev;
        ESP_LOGD(TAG, "Created device handle for address 0x%02X", dev_addr);
    } else {
        ESP_LOGE(TAG, "Failed to add device at 0x%02X: %s", dev_addr, esp_err_to_name(err));
    }
    return err;
}

esp_err_t I2CBus::write(uint8_t dev_addr, const uint8_t *data, size_t len)
{
    i2c_master_dev_handle_t dev;
    ESP_RETURN_ON_ERROR(get_device(dev_addr, &dev), TAG, "get_device failed");
    return i2c_master_transmit(dev, data, len, -1);
}

esp_err_t I2CBus::read(uint8_t dev_addr, uint8_t *data, size_t len)
{
    i2c_master_dev_handle_t dev;
    ESP_RETURN_ON_ERROR(get_device(dev_addr, &dev), TAG, "get_device failed");
    return i2c_master_receive(dev, data, len, -1);
}

esp_err_t I2CBus::write_then_read(uint8_t dev_addr,
                                  const uint8_t *write_data, size_t write_len,
                                  uint8_t *read_data, size_t read_len)
{
    i2c_master_dev_handle_t dev;
    ESP_RETURN_ON_ERROR(get_device(dev_addr, &dev), TAG, "get_device failed");
    return i2c_master_transmit_receive(dev, write_data, write_len, read_data, read_len, -1);
}