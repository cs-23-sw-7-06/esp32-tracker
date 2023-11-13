#pragma once

#include <memory>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <esp_check.h>
#include <esp_err.h>
#include <esp_event.h>
#include <esp_wifi.h>

enum class WifiConnectionState {
  Starting,
  Connected,
  Stopping,
  Stopped,
};

class WifiConnection {
private:
  WifiConnectionState m_wifi_connection_state;
  SemaphoreHandle_t m_semaphore;
  esp_event_handler_instance_t m_wifi_event_handler_instance;
  esp_event_handler_instance_t m_ip_event_handler_instance;

  static void wifi_ip_event_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data);

public:
  WifiConnection();
  WifiConnection(const WifiConnection &) = delete;

  ~WifiConnection();

  static void setup_wifi();
};
