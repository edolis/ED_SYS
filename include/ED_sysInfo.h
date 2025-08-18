#pragma once

#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_mac.h"
#include <esp_partition.h>


namespace ED_SYSINFO
{

    //TODO add chip temperature sensing , section 34.3 of the ESP32C3 technical documentation

/// @brief lists the partition configuration of the NVS
void print_partitions() ;
void dumpSysInfo();
void print_dns_info();
void dump_ca_cert(const uint8_t *ca_crt_start,const uint8_t *ca_crt_end);
void DNSlookup(std::string nodeName) ;
}//namespace