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

  const auto wifi_mac_address = WifiConnection::get_mac_address();
  const auto bluetooth_mac_address = BluetoothClassicScanner::get_mac_address();

  WifiConnection::setup_wifi();

  while (true) {
    const auto freeHeap = xPortGetFreeHeapSize();
    ESP_LOGI("main", "Free heap: %zuKB", freeHeap / 1000);
    bool registered = false;
    int tracker_id = -1;

    {
      WifiConnection connection;

      do {
        char url_string[256];

        snprintf(
            url_string, sizeof(url_string),
            "http://" API_HOSTNAME ":" API_PORT "/tracker/registration/info"
            "?wifiMacAddress=%02x%%3A%02x%%3A%02x%%3A%02x%%3A%02x%%3A%02x"
            "&bluetoothMacAddress=%02x%%3A%02x%%3A%02x%%3A%02x%%3A%02x%%3A%02x",
            wifi_mac_address.bytes[0], wifi_mac_address.bytes[1],
            wifi_mac_address.bytes[2], wifi_mac_address.bytes[3],
            wifi_mac_address.bytes[4], wifi_mac_address.bytes[5],
            bluetooth_mac_address.bytes[0], bluetooth_mac_address.bytes[1],
            bluetooth_mac_address.bytes[2], bluetooth_mac_address.bytes[3],
            bluetooth_mac_address.bytes[4], bluetooth_mac_address.bytes[5]);

        printf("%s", url_string);

        HttpRequest getRegistrationInfoRequest(url_string, HTTP_METHOD_GET);
        const auto responseText = getRegistrationInfoRequest.send();
        printf("%s\n", responseText.data());

        cJSON *json_root = cJSON_Parse(responseText.data());
        cJSON *json_registered = cJSON_GetObjectItem(json_root, "registered");

        registered = cJSON_IsTrue(json_registered);

        if (registered) {
          cJSON *json_tracker_id = cJSON_GetObjectItem(json_root, "trackerId");
          tracker_id = cJSON_GetNumberValue(json_tracker_id);
        }

        cJSON_Delete(json_root);

        if (!registered) {
          ESP_LOGI("main", "We are not registered!");
          vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
      } while (!registered);

      ESP_LOGI("main", "Registered! Tracker Id: %d", tracker_id);
    }

    std::vector<LocalizationTarget> localization_targets;
    {
      WifiSniffer sniffer;
      localization_targets = sniffer.sniff(wifi_sniffer_sniff_time);
    }

    {
      BluetoothClassicScanner bluetooth_classic_scanner;
      const auto bluetooth_localization_targets =
          bluetooth_classic_scanner.scan(bluetooth_classic_scanner_scan_time);
      localization_targets.insert(localization_targets.end(),
                                  bluetooth_localization_targets.begin(),
                                  bluetooth_localization_targets.end());
    }

    // Send this shite
    {
      const auto json_data =
          serialize_localization_targets(localization_targets, tracker_id);
      ESP_LOGI("main", "%s", json_data.c_str());

      WifiConnection connection;

      HttpRequest add_measurements_request{
          "http://" API_HOSTNAME ":" API_PORT "/measurements/add",
          HTTP_METHOD_POST};

      auto response = add_measurements_request.send_with_post_data(json_data);
      ESP_LOGI("main", "%s", response.c_str());
    }
  }
}
