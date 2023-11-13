#include "HttpRequest.hpp"
#include "WifiConnection.hpp"
#include "WifiSniffer.hpp"
#include "wifi.hpp"

#include "sdkconfig.h"
#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <inttypes.h>
#include <string.h>

extern "C" void app_main(void) {
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
      const auto localization_targets = sniffer.sniff(10000);
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
