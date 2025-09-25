#include "ED_sysInfo.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include <lwip/netdb.h>
#include <sstream>
#include <vector>

namespace ED_SYSINFO {

static const char *TAG = "ED_SYSINFO";
static const char *esp_mac_type_str[] = {
    "ESP_MAC_WIFI_STA",      "ESP_MAC_WIFI_SOFTAP",  "ESP_MAC_BT",
    "ESP_MAC_ETH",           "ESP_MAC_IEEE802154",   "ESP_MAC_BASE",
    "ESP_MAC_EFUSE_FACTORY", "ESP_MAC_EFUSE_CUSTOM", "ESP_MAC_EFUSE_EXT"};

// ===== fwInfo =====
fwInfo::VersionInfo *fwInfo::_Vinfo = nullptr;

bool fwInfo::VersionInfo::isEmpty() { return full.empty(); }

const char *fwInfo::PrjName() { return getDesc()->project_name; }
const char *fwInfo::AppCompileDate() { return getDesc()->date; }
const fwInfo::VersionInfo &fwInfo::AppVers() { return *_Vinfo; }
const char *fwInfo::AppCompileTime() { return getDesc()->time; }

const char *fwInfo::AppELFSha256() {
  static char sha256_str[65] = "";
  if (sha256_str[0] == '\0') {
    for (int i = 0; i < 32; ++i) {
      sprintf(&sha256_str[i * 2], "%02x", getDesc()->app_elf_sha256[i]);
    }
  }
  return sha256_str;
}

const char *fwInfo::AppESPIDF() { return esp_get_idf_version(); }
const char *fwInfo::_AppVersStr() { return getDesc()->version; }

fwInfo::VersionInfo &fwInfo::parseVersion(const char *versionStr) {
  static fwInfo::VersionInfo info;
  info.isValidGIT = true;
  if (!versionStr)
    return info;

  info.full = versionStr;
  std::string ver = versionStr;
  if (ver[0] == 'v')
    ver = ver.substr(1);
  else
    info.isValidGIT = false;

  std::vector<std::string> parts;
  std::stringstream ss(ver);
  std::string item;
  while (std::getline(ss, item, '-'))
    parts.push_back(item);

  sscanf(parts[0].c_str(), "%d.%d.%d", &info.major, &info.minor, &info.patch);
  if (parts.size() > 1 && std::isdigit(parts[1][0]))
    info.commitsAhead = std::stoi(parts[1]);
  if (parts.size() > 2)
    info.GITcommitID = parts[2];
  if (parts.size() > 3 && parts[3] == "dirty")
    info.isDirty = true;
  return info;
}

const esp_app_desc_t *fwInfo::getDesc() {
  static const esp_app_desc_t *desc = nullptr;
  if (!desc) {
    desc = esp_app_get_description();
    _Vinfo = &parseVersion(desc->version);
  }
  return desc;
}

// ===== MacAddress =====
MacAddress::MacAddress() { memset(_mac_addr, 0, 6); }
MacAddress::MacAddress(const uint8_t mac[6]) : MacAddress() { set(mac); }
const uint8_t &MacAddress::operator[](size_t index) const {
  return _mac_addr[index];
}
const uint8_t *MacAddress::get() const { return _mac_addr; }
uint8_t *MacAddress::set(const uint8_t mac[6]) {
  if (!isZeroMac(mac)) {
    _isValid = true;
    memcpy(_mac_addr, mac, 6);
  }
  return _mac_addr;
}
char *MacAddress::toString(char *buffer, size_t buffer_size,
                           char separator) const {
  if (buffer && buffer_size >= 18) {
    snprintf(buffer, buffer_size, "%02X%c%02X%c%02X%c%02X%c%02X%c%02X",
             _mac_addr[0], separator, _mac_addr[1], separator, _mac_addr[2],
             separator, _mac_addr[3], separator, _mac_addr[4], separator,
             _mac_addr[5]);
  }
  return buffer;
}
bool MacAddress::isValid() const { return _isValid; }
bool MacAddress::isZeroMac(const uint8_t mac[6]) const {
  return memcmp(mac, zeroMac, 6) == 0;
}

// ===== ESP_MACstorage =====
std::mutex ESP_MACstorage::macMapMutex;
std::unordered_map<esp_mac_type_t, MacAddress> ESP_MACstorage::ESPmacMap{};
std::once_flag ESP_MACstorage::macInitFlag;

void ESP_MACstorage::initListener() {
  esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &on_wifi_start,
                             NULL);
  esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_START, &on_wifi_start,
                             NULL);
}

MacAddress ESP_MACstorage::getMac(esp_mac_type_t type) {
  ensureMacsInitialized();
  std::lock_guard<std::mutex> lock(macMapMutex);
  auto it = ESPmacMap.find(type);
  return (it != ESPmacMap.end()) ? it->second : MacAddress();
}

const std::unordered_map<esp_mac_type_t, MacAddress> &
ESP_MACstorage::getMacMap() {
  return ESPmacMap;
}

void ESP_MACstorage::initMacs() {
  for (int i = 0; i < static_cast<int>(sizeof(esp_mac_type_str) /
                                       sizeof(esp_mac_type_str[0]));
       ++i) {
    esp_mac_type_t type = static_cast<esp_mac_type_t>(i);

    // Skip unsupported or non-6-byte MAC types
    if (type == ESP_MAC_IEEE802154 || type == ESP_MAC_EFUSE_EXT) {
      continue;
    }

    uint8_t mac[6] = {};
    esp_err_t err = esp_read_mac(mac, type);
    if (err == ESP_OK) {
      ESP_LOGI("MACstorage", "assigning to internal ESPmacMap MAC type: %d",
               static_cast<int>(type));
      std::lock_guard<std::mutex> lock(macMapMutex);
      ESPmacMap[type] = MacAddress(mac);
    } else {
      ESP_LOGW("ESP_MACstorage", "Could not read MAC for type %s",
               esp_mac_type_str[i]);
    }
  }
}

void ESP_MACstorage::on_wifi_start(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data) {
  uint8_t mac[6] = {};
  esp_err_t err = esp_wifi_get_mac(
      (event_id == WIFI_EVENT_STA_START) ? WIFI_IF_STA : WIFI_IF_AP, mac);

  if (err == ESP_OK) {
    std::lock_guard<std::mutex> lock(macMapMutex);
    ESPmacMap[(event_id == WIFI_EVENT_STA_START)
                  ? esp_mac_type_t::ESP_MAC_WIFI_STA
                  : esp_mac_type_t::ESP_MAC_WIFI_SOFTAP] = MacAddress(mac);
  } else {
    ESP_LOGW("ESP_MACstorage", "esp_wifi_get_mac failed: %s",
             esp_err_to_name(err));
  }
}

void ESP_MACstorage::ensureMacsInitialized() {
  std::call_once(macInitFlag, []() { ESP_MACstorage::initMacs(); });
}

// ==================== Free functions ====================

void DNSlookup(const char *nodeName) {
  struct addrinfo hints = {};
  struct addrinfo *res = nullptr;
  int err = getaddrinfo(nodeName, NULL, &hints, &res);
  if (err != 0 || res == NULL) {
    ESP_LOGE(TAG, "DNS lookup failed for %s", nodeName);
    return;
  }
  struct sockaddr_in *addr =
      reinterpret_cast<struct sockaddr_in *>(res->ai_addr);
  ESP_LOGI(TAG, "%s resolved to: %s", nodeName, inet_ntoa(addr->sin_addr));
  freeaddrinfo(res);
}

void dump_ca_cert(const uint8_t *ca_crt_start, const uint8_t *ca_crt_end) {
  static const char *CERT_TAG = "CA_CERT_DUMP";
  const uint8_t *start = ca_crt_start;
  const uint8_t *end = ca_crt_end;
  size_t len = static_cast<size_t>(end - start);

  ESP_LOGI(CERT_TAG, "CA certificate length: %d bytes", static_cast<int>(len));

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

  uint32_t flash_size = 0;
  if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
    ESP_LOGI(TAG, "Flash size: %dMB", flash_size / (1024 * 1024));
  } else {
    ESP_LOGW(TAG, "Failed to get flash size");
  }

  uint32_t flash_id = 0;
  if (esp_flash_read_id(NULL, &flash_id) == ESP_OK) {
    ESP_LOGI(TAG, "Flash ID: 0x%08X", flash_id);
  } else {
    ESP_LOGW(TAG, "Failed to read flash ID");
  }

  uint8_t mac[6] = {};
  if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
    ESP_LOGI(TAG, "MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2],
             mac[3], mac[4], mac[5]);
  } else {
    ESP_LOGW(TAG, "Failed to read STA MAC");
  }
}

} // namespace ED_SYSINFO
