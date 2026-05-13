#include "ed_i2c.h"
#include "esp_log.h"
#include <cstring>

static const char *TAG = "ed_i2c";

bool I2CBus::s_installed[I2C_NUM_MAX] = { false };

I2CBus::I2CBus(i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t freq)
{
    if (!s_installed[port]) {
        i2c_master_bus_config_t bus_cfg;
        memset(&bus_cfg, 0, sizeof(bus_cfg));
        bus_cfg.i2c_port = port;
        bus_cfg.sda_io_num = sda;
        bus_cfg.scl_io_num = scl;
        bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
        bus_cfg.glitch_ignore_cnt = 7;
        bus_cfg.flags.enable_internal_pullup = true;  // enable internal pull‑ups

        ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &m_bus_handle));
        s_installed[port] = true;
        ESP_LOGI(TAG, "Bus %d initialised at %lu Hz", port, freq);
    } else {
        // Bus already exists – get the handle? We'll just leave m_bus_handle invalid.
        // You can extend to return existing handle, but for single‑port use this is fine.
        m_bus_handle = nullptr;
    }
}

I2CBus::~I2CBus() {
    // In a typical app the bus lives forever; if you need cleanup, call
    // i2c_del_master_bus(m_bus_handle). Left out for simplicity.
}