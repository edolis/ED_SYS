#ifndef ED_I2C_H
#define ED_I2C_H

#include "hal/gpio_types.h"
#include "driver/i2c_master.h"   // new driver

class I2CBus {
public:
    /**
     * @param port   I²C port number (I2C_NUM_0, etc.)
     * @param sda    GPIO for SDA
     * @param scl    GPIO for SCL
     * @param freq   Clock speed in Hz (default 400 kHz)
     */
    I2CBus(i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t freq = 400000);
    ~I2CBus();                             // now cleans up the bus handle

    I2CBus(const I2CBus&) = delete;
    I2CBus& operator=(const I2CBus&) = delete;

    i2c_master_bus_handle_t handle() const { return m_bus_handle; }

private:
    i2c_master_bus_handle_t m_bus_handle;
    static bool s_installed[I2C_NUM_MAX];
};

#endif