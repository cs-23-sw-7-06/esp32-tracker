#include "BluetoothClassicScanner.hpp"
#include "HttpRequest.hpp"
#include "WifiConnection.hpp"
#include "WifiSniffer.hpp"
#include "wifi.hpp"

#include "sdkconfig.h"

#include <inttypes.h>

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs.h>
#include <nvs_flash.h>

#include <cJSON.h>

#define TO_STR(int) #int
#define XTO_STR(int) TO_STR(int)

#define API_HOSTNAME CONFIG_API_HOSTNAME
#define API_PORT XTO_STR(CONFIG_API_PORT)

static constexpr auto wifi_sniffer_sniff_time = CONFIG_WIFI_SNIFFER_SNIFF_TIME;

static constexpr auto bluetooth_classic_scanner_scan_time =
    CONFIG_BLUETOOTH_CLASSIC_SCANNER_SCAN_TIME;

static void setup_nvs_flash() {
  auto ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }

  ESP_ERROR_CHECK(ret);
}

static std::string serialize_localization_targets(
    const std::vector<LocalizationTarget> &localization_targets,
    int tracker_id) {
  auto *json_object = cJSON_CreateObject();
  cJSON_AddNumberToObject(json_object, "trackerId", tracker_id);
  auto *json_measurements = cJSON_AddArrayToObject(json_object, "measurements");

  for (const auto &target : localization_targets) {
    const auto &address = target.m_mac_address;
    char mac_address[128];
    snprintf(mac_address, sizeof(mac_address), "%02x:%02x:%02x:%02x:%02x:%02x",
             address.bytes[0], address.bytes[1], address.bytes[2],
             address.bytes[3], address.bytes[4], address.bytes[5]);

    auto *json_measurement_item = cJSON_CreateObject();
    cJSON_AddStringToObject(json_measurement_item, "macAddress", mac_address);
    cJSON_AddNumberToObject(json_measurement_item, "type",
                            (int)target.m_sniffer_type);
    cJSON_AddNumberToObject(json_measurement_item, "rssi", target.m_rssi);

    cJSON_AddItemToArray(json_measurements, json_measurement_item);
  }
  auto *json_string = cJSON_Print(json_object);
  std::string json_data{json_string};
  free(json_string);

  cJSON_Delete(json_object);

  return json_data;
}

extern "C" void app_main(void) {
  setup_nvs_flash();

  esp_event_loop_create_default();

  WifiConnection::setup_wifi();

  while (true) {
    {
      WifiSniffer sniffer;
      const auto localization_targets = sniffer.sniff(wifi_sniffer_sniff_time);
      for (auto& localization_target : localization_targets) {
        auto& m = localization_target.m_mac_address.bytes;
        ESP_LOGI("main", "WIFI: %02x:%02x:%02x:%02x:%02x:%02x RSSI: %d", m[0], m[1], m[2], m[3], m[4], m[5], localization_target.m_rssi);
      }
    }

    {
      BluetoothClassicScanner bluetooth_classic_scanner;
      const auto bluetooth_localization_targets =
          bluetooth_classic_scanner.scan(bluetooth_classic_scanner_scan_time);

      for (auto& localization_target : bluetooth_localization_targets) {
        auto& m = localization_target.m_mac_address.bytes;
        ESP_LOGI("main", "BT: %02x:%02x:%02x:%02x:%02x:%02x RSSI: %d", m[0], m[1], m[2], m[3], m[4], m[5], localization_target.m_rssi);
      }
    }
  }
}
