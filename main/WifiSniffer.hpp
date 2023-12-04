#pragma once

#include "LocalizationTarget.hpp"

#include <unordered_set>
#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <esp_wifi.h>

class WifiSniffer {
private:
  static void packet_callback(void *buf, wifi_promiscuous_pkt_type_t type);
  static WifiSniffer *m_instance;
  std::vector<LocalizationTarget> m_localization_targets;

  // Channel hopper stuff
  static std::unordered_set<int> scan_for_used_channels();
  static void channel_hopper_task(void *parameters);
  int m_timeout_ms;
  std::unordered_set<int> m_channels;
  TaskHandle_t m_channel_hopper_task;
  SemaphoreHandle_t m_channel_hopper_should_exit_semaphore;
  SemaphoreHandle_t m_channel_hopper_exited_semaphore;

public:
  // Assumption: We dont have an active WifiConnection
  WifiSniffer();
  ~WifiSniffer();

  // Sniff localization targets for an amount of time in miliseconds.
  std::vector<LocalizationTarget> sniff(int timeout_ms);
};
