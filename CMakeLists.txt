idf_component_register(
    SRCS "ED_heap_audit.c" "ED_sysInfo.cpp" "ED_PC_wrapper.cpp"
    "ED_esp_err.cpp" "ED_sys.cpp" "ED_SNTP_time.cpp" "ed_i2c.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        esp_netif
        esp_wifi
        esp_timer
        app_update
        spi_flash
        diag
        driver
)
