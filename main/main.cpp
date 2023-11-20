#include "BluetoothClassicScanner.hpp"
#include "HttpRequest.hpp"
#include "WifiConnection.hpp"
#include "WifiSniffer.hpp"
#include "wifi.hpp"

#include "sdkconfig.h"

#include "sdkconfig.h"
#include <inttypes.h>

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs.h>
#include <nvs_flash.h>

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

extern "C" void app_main(void) {
  setup_nvs_flash();

  esp_event_loop_create_default();

  WifiConnection::setup_wifi();

  while (true) {
    {
      WifiConnection connection;

      {
        HttpRequest createRoomRequest("http://10.0.0.1:8081/room/add",
                                      HTTP_METHOD_POST);
        const auto response = createRoomRequest.send_with_post_data(
            R"(
          { "RoomId": 0,
            "Name": "Created from esp",
            "Description": "Fuck you!"
          })");
        printf("%s\n", response.data());
      }

      HttpRequest request{"http://10.0.0.1:8081/room/all", HTTP_METHOD_GET};
      const auto str = request.send();
      printf("%s\n", str.data());
    }

    {
      WifiSniffer sniffer;
      const auto localization_targets = sniffer.sniff(wifi_sniffer_sniff_time);
      for (const auto &target : localization_targets) {
        const auto &address = target.m_mac_address;
        printf("mac: %02x:%02x:%02x:%02x:%02x:%02x type: %s rssi: %d\n",
               address.bytes[0], address.bytes[1], address.bytes[2],
               address.bytes[3], address.bytes[4], address.bytes[5],
               target.m_sniffer_type == SnifferType::WiFi ? "WiFi"
                                                          : "Bluetooth",
               target.m_rssi);
      }
    }

    {
      BluetoothClassicScanner bluetooth_classic_scanner;
      const auto localization_targets =
          bluetooth_classic_scanner.scan(bluetooth_classic_scanner_scan_time);
      for (const auto &target : localization_targets) {
        const auto &address = target.m_mac_address;
        printf("mac: %02x:%02x:%02x:%02x:%02x:%02x type: %s rssi: %d\n",
               address.bytes[0], address.bytes[1], address.bytes[2],
               address.bytes[3], address.bytes[4], address.bytes[5],
               target.m_sniffer_type == SnifferType::WiFi ? "WiFi"
                                                          : "Bluetooth",
               target.m_rssi);
      }
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}
