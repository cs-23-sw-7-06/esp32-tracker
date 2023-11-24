#pragma once

#include "LocalizationTarget.hpp"

#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <esp_gap_bt_api.h>

class BluetoothClassicScanner {
  static BluetoothClassicScanner *m_instance;
  SemaphoreHandle_t m_semaphore;
  std::vector<LocalizationTarget> m_localization_targets;

  static void gap_callback(esp_bt_gap_cb_event_t event,
                           esp_bt_gap_cb_param_t *param);

public:
  std::vector<LocalizationTarget> scan(int timeout_ms);

  BluetoothClassicScanner();
  BluetoothClassicScanner(const BluetoothClassicScanner &) = delete;
  ~BluetoothClassicScanner();

  static MacAddress get_mac_address();
};
