#include "ED_sysInfo.h"
#include "esp_netif.h"
#include <esp_netif_types.h>
#include <lwip/netdb.h>
#include <string>

namespace ED_SYSINFO {
static const char *TAG = "ED_SYSINFO";
void DNSlookup(std::string nodeName) {
  struct addrinfo hints = {};
  struct addrinfo *res;
  int err = getaddrinfo(nodeName.c_str(), NULL, &hints, &res);
  if (err != 0 || res == NULL) {
    ESP_LOGE(TAG, "DNS lookup failed for %s", nodeName.c_str());
  } else {
    struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
    ESP_LOGI(TAG, "%s resolved to: %s", nodeName.c_str(),
             inet_ntoa(addr->sin_addr));
    freeaddrinfo(res);
  }
}
void dump_ca_cert(const uint8_t *ca_crt_start, const uint8_t *ca_crt_end) {
  static const char *CERT_TAG = "CA_CERT_DUMP";
  const uint8_t *start = ca_crt_start;
  const uint8_t *end = ca_crt_end;
  size_t len = end - start;

  ESP_LOGI(CERT_TAG, "CA certificate length: %d bytes", (int)len);

  std::string cert(reinterpret_cast<const char *>(start), len);
  size_t pos = 0;
  while (pos < cert.size()) {
    size_t next = cert.find('\n', pos);
    if (next == std::string::npos)
      next = cert.size();
    std::string line = cert.substr(pos, next - pos);
    ESP_LOGI(CERT_TAG, "%s", line.c_str());
    pos = next + 1;
  }
}

void print_dns_info() {
  esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if (netif == NULL) {
    ESP_LOGE("DNS", "Failed to get netif handle");
    return;
  }

  esp_netif_dns_info_t dns_info;
  esp_err_t err = esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);
  if (err != ESP_OK) {
    ESP_LOGE("DNS", "Failed to get DNS info: %s", esp_err_to_name(err));
    return;
  }

  ESP_LOGI("DNS", "Main DNS Server IP: " IPSTR,
           IP2STR(&dns_info.ip.u_addr.ip4));
}

void print_partitions() {
  esp_partition_iterator_t it = esp_partition_find(
      ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
  while (it != NULL) {
    const esp_partition_t *part = esp_partition_get(it);
    ESP_LOGI("PARTITION", "App Partition: %s at 0x%06x, size: 0x%x",
             part->label, part->address, part->size);
    it = esp_partition_next(it);
  }
  esp_partition_iterator_release(it);
}

void dumpSysInfo() {
  // Chip Info
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  ESP_LOGI(TAG, "Chip revision: v%d.%d", chip_info.revision / 100,
           chip_info.revision % 100);

  uint32_t flash_size;
  esp_flash_get_size(NULL, &flash_size);
  ESP_LOGI(TAG, "Flash size: %dMB", flash_size / (1024 * 1024));

  uint32_t flash_id;
  esp_flash_read_id(NULL, &flash_id);
  ESP_LOGI(TAG, "Flash ID: 0x%08X", flash_id);

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  ESP_LOGI(TAG, "MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);
}

} // namespace ED_SYSINFO
