#ifndef ED_I2C_H
#define ED_I2C_H

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "driver/gpio.h"  // for gpio_num_t
#include <map>

class I2CBus {
public:
    /**
     * Constructor – initializes the I2C master bus (if not already done).
     * @param port I2C port number (I2C_NUM_0 or I2C_NUM_1)
     * @param sda GPIO pin for SDA
     * @param scl GPIO pin for SCL
     * @param freq Clock frequency in Hz (e.g., 400000)
     */
    I2CBus(i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t freq);
    ~I2CBus();

    /**
     * Obtain a device handle for a given slave address.
     * The handle is cached and reused on subsequent calls.
     * @param dev_addr 7‑bit I2C address of the slave
     * @param dev_handle output pointer to store the device handle
     * @return ESP_OK on success, otherwise an error code
     */
    esp_err_t get_device(uint8_t dev_addr, i2c_master_dev_handle_t *dev_handle);

    // Convenience wrappers using the cached device handle
    esp_err_t write(uint8_t dev_addr, const uint8_t *data, size_t len);
    esp_err_t read(uint8_t dev_addr, uint8_t *data, size_t len);
    esp_err_t write_then_read(uint8_t dev_addr,
                              const uint8_t *write_data, size_t write_len,
                              uint8_t *read_data, size_t read_len);

private:
    i2c_master_bus_handle_t m_bus_handle;
    std::map<uint8_t, i2c_master_dev_handle_t> m_devices;  // cache of device handles
    uint32_t m_freq;  // store frequency for device creation

    static bool s_installed[I2C_NUM_MAX];
};

#endif // ED_I2C_H