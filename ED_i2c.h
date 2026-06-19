#ifndef ED_I2C_H
#define ED_I2C_H

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include <map>
#include <functional>
#include <cstddef>

class I2CBus {
public:
    I2CBus(i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t freq);
    ~I2CBus();

    esp_err_t write(uint8_t dev_addr, const uint8_t *data, size_t len);
    esp_err_t read(uint8_t dev_addr, uint8_t *data, size_t len);
    esp_err_t write_then_read(uint8_t dev_addr,
                              const uint8_t *write_data, size_t write_len,
                              uint8_t *read_data, size_t read_len);
    esp_err_t probe(uint8_t dev_addr, uint32_t timeout_ms = 100);

    esp_err_t register_recovery_callback(uint8_t dev_addr, std::function<esp_err_t(void)> callback);

private:
    esp_err_t get_device(uint8_t dev_addr, i2c_master_dev_handle_t *dev_handle);
    esp_err_t recover_bus();

    i2c_port_t      m_port;
    gpio_num_t      m_sda;
    gpio_num_t      m_scl;
    uint32_t        m_freq;

    i2c_master_bus_handle_t   m_bus_handle;
    std::map<uint8_t, i2c_master_dev_handle_t> m_devices;
    std::map<uint8_t, std::function<esp_err_t(void)>> m_recovery_callbacks;
    std::map<uint8_t, int> m_failure_count;   // per‑device consecutive failures
};

#endif // ED_I2C_H