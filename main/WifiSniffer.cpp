#include "WifiSniffer.hpp"

#include "sdkconfig.h"

#include <cstring>

#include <esp_err.h>
#include <esp_log.h>

static const char *wifi_sniffer_tag = "wifi-sniffer";

constexpr static auto channel_hopper_frequency =
    CONFIG_CHANNEL_HOPPER_FREQUENCY; // ms

WifiSniffer *WifiSniffer::m_instance = nullptr;

void WifiSniffer::packet_callback(void *buf, wifi_promiscuous_pkt_type_t type) {
  auto pkt = (wifi_promiscuous_pkt_t *)buf;
  auto *frame_control = (FrameControl *)pkt->payload;

  if (frame_control->type == FrameControlType::Management) {
    auto *management_frame = (ManagementFrame *)pkt->payload;

    switch ((ManagementSubType)management_frame->frame_control.subtype) {
    // These are the only two that are only sent by localization target (eg. not
    // an AP)
    case ManagementSubType::AssociationRequest:
    case ManagementSubType::ProbeRequest:
      ESP_LOGI(wifi_sniffer_tag, "Got a Probe Request packet");

      printf("Destination address: ");
      print_mac_address(management_frame->destination_address);

      printf("Transmitter address: ");
      print_mac_address(management_frame->transmitter_address);

      // TODO: Lock vector
      m_instance->m_localization_targets.push_back(
          {management_frame->transmitter_address, SnifferType::WiFi,
           pkt->rx_ctrl.rssi});

    default:
      break;
    }

    return;
  }
  if (frame_control->type == FrameControlType::Data) {
    ESP_LOGI(wifi_sniffer_tag, "Got a Data packet");

    // We can use all Data types as we have a flag that says which direction the
    // package is sent (eg. from STA to AP).
    const auto *data_frame = (const DataFrame *)pkt->payload;

    if (!data_frame->toAP()) {
      // Not sent by a localization target.
      return;
    }

    printf("Channel: %d\n", pkt->rx_ctrl.channel);
    print_data_subtype(data_frame->frame_control);

    printf("Receiver address: ");
    print_mac_address(data_frame->receiver_address);

    printf("Transmitter address: ");
    print_mac_address(data_frame->transmitter_address);

    printf("Destination address: ");
    print_mac_address(data_frame->destination_address);

    printf("Source address: ");
    print_mac_address(data_frame->source_address);

    m_instance->m_localization_targets.push_back(
        {data_frame->transmitter_address, SnifferType::WiFi,
         pkt->rx_ctrl.rssi});

    return;
  }
}
std::unordered_set<int> WifiSniffer::scan_for_used_channels() {
  std::unordered_set<int> channels;
  uint16_t number = 64;
  wifi_ap_record_t *ap_info = new wifi_ap_record_t[number];
  uint16_t ap_count = 0;
  memset(ap_info, 0, number * sizeof(wifi_ap_record_t));

  esp_wifi_scan_start(NULL, true);
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
  ESP_LOGI(wifi_sniffer_tag,
           "Total APs scanned = %u, actual AP number ap_info holds = %u",
           ap_count, number);
  for (int i = 0; i < number; i++) {
    ESP_LOGI(wifi_sniffer_tag, "SSID \t\t%s", ap_info[i].ssid);
    ESP_LOGI(wifi_sniffer_tag, "Channel \t\t%d", ap_info[i].primary);
    channels.insert(ap_info[i].primary);
  }

  // Free the ap records
  ESP_ERROR_CHECK(esp_wifi_clear_ap_list());

  // Free our ap records
  delete[] ap_info;

  // If we could not find any APs just add 1 as a channel.
  if (channels.empty()) {
    channels.insert(1);
  }

  return channels;
}

void WifiSniffer::channel_hopper_task(void *parameters) {
  const auto *thiz = (const WifiSniffer *)parameters;

  while (true) {
    for (const auto channel : thiz->m_channels) {
      if (xSemaphoreTake(thiz->m_channel_hopper_should_exit_semaphore,
                         channel_hopper_frequency / portTICK_PERIOD_MS) ==
          pdTRUE) {
        // Signal that it is safe to continue
        xSemaphoreGive(thiz->m_channel_hopper_exited_semaphore);
        // Stop this task
        vTaskDelete(NULL);
      }

      ESP_LOGI(wifi_sniffer_tag, "Switching channel to %d", channel);
      esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    }
  }
}

WifiSniffer::WifiSniffer() {
  m_instance = this;
  m_channel_hopper_should_exit_semaphore = xSemaphoreCreateCounting(1, 0);
  m_channel_hopper_exited_semaphore = xSemaphoreCreateCounting(1, 0);

  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(packet_callback));
  m_channels = scan_for_used_channels();
}

WifiSniffer::~WifiSniffer() {
  ESP_ERROR_CHECK(esp_wifi_stop());
  vSemaphoreDelete(m_channel_hopper_exited_semaphore);
  vSemaphoreDelete(m_channel_hopper_should_exit_semaphore);
  m_instance = nullptr;
}

std::vector<LocalizationTarget> WifiSniffer::sniff(int timeout_ms) {
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

  // Create channel hopper task
  xTaskCreate(channel_hopper_task, "Channel Hopper", 2048, this,
              tskIDLE_PRIORITY, &m_channel_hopper_task);

  // Sniff for this many miliseconds
  vTaskDelay(timeout_ms / portTICK_PERIOD_MS);

  // Tell the channel hopper to exit
  xSemaphoreGive(m_channel_hopper_should_exit_semaphore);

  // Wait for channel hopper to exit
  xSemaphoreTake(m_channel_hopper_exited_semaphore, portMAX_DELAY);

  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));

  // TODO: Lock vector
  return m_localization_targets;
}
