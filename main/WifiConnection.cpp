#include "WifiConnection.hpp"

#include "sdkconfig.h"

#include <string.h>

#include <esp_err.h>
#include <esp_event.h>
#include <esp_mac.h>

constexpr static auto wifi_ssid = CONFIG_WIFI_SSID;
constexpr static auto wifi_password = CONFIG_WIFI_PASSWORD;
constexpr static auto wifi_retry_count = CONFIG_WIFI_RETRY_COUNT;

constexpr static auto wifi_tag = "wifi";

// FIXME: This sucks we should use a semaphore but it sucks
volatile static bool handler_running = false;

void WifiConnection::wifi_ip_event_handler(void *arg,
                                           esp_event_base_t event_base,
                                           int32_t event_id, void *event_data) {
  auto thiz = (WifiConnection *)arg;
  if (event_base == IP_EVENT) {
    switch (event_id) {
    case IP_EVENT_STA_GOT_IP:
      ESP_LOGI(wifi_tag, "GOT IP!");
      handler_running = false;
      thiz->m_wifi_connection_state = WifiConnectionState::Connected;
      xSemaphoreGive(thiz->m_semaphore);
    }
  } else {
    switch (event_id) {
    case WIFI_EVENT_STA_START:
      ESP_LOGI(wifi_tag, "WIFI STARTED");
      if (thiz->m_wifi_connection_state != WifiConnectionState::Starting) {
        ESP_LOGE(wifi_tag, "WTF JUST HAPPENED!!?!!?! RESTARTING!!!!");
        esp_restart();
        return;
      }
      ESP_ERROR_CHECK(esp_wifi_connect());
      break;
    case WIFI_EVENT_STA_CONNECTED:
      ESP_LOGI(wifi_tag, "WIFI CONNECTED");
      break;
    case WIFI_EVENT_STA_STOP:
      ESP_LOGI(wifi_tag, "WIFI STOPPED");
      thiz->m_wifi_connection_state = WifiConnectionState::Stopped;
      xSemaphoreGive(thiz->m_semaphore);
      break;
    case WIFI_EVENT_STA_DISCONNECTED:
      ESP_LOGI(wifi_tag, "WIFI_DISCONNECTED");
      if (thiz->m_wifi_connection_state != WifiConnectionState::Stopping) {
        ESP_LOGE(wifi_tag, "WTF JUST HAPPENED!!?!!?! RESTARTING!!!!");
        esp_restart();
        return;
      }
      ESP_ERROR_CHECK(esp_wifi_stop());
      break;
    }
  }
}

void WifiConnection::setup_wifi() {
  ESP_ERROR_CHECK(esp_netif_init());
  const auto *sta_netif = esp_netif_create_default_wifi_sta();
  if (!sta_netif) {
    ESP_LOGE(wifi_tag, "esp_netif_create_default_wifi_sta() failed. Aborting!");
    abort();
  }

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  wifi_config_t wifi_cfg;
  ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg));

  // Casting to char* is probably undefined behavior but i dont give a damn
  strncpy((char *)wifi_cfg.sta.ssid, wifi_ssid, sizeof(wifi_cfg.sta.ssid));
  strncpy((char *)wifi_cfg.sta.password, wifi_password,
          sizeof(wifi_cfg.sta.password));

  wifi_cfg.sta.failure_retry_cnt = wifi_retry_count;

  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
}

WifiConnection::WifiConnection() {
  m_wifi_connection_state = WifiConnectionState::Starting;
  m_semaphore = xSemaphoreCreateCounting(1, 0);
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_ip_event_handler, this,
      &m_wifi_event_handler_instance));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, ESP_EVENT_ANY_ID, wifi_ip_event_handler, this,
      &m_ip_event_handler_instance));
  ESP_ERROR_CHECK(esp_wifi_start());
  xSemaphoreTake(m_semaphore, portMAX_DELAY);
}

WifiConnection::~WifiConnection() {
  m_wifi_connection_state = WifiConnectionState::Stopping;
  ESP_ERROR_CHECK(esp_wifi_disconnect());
  xSemaphoreTake(m_semaphore, portMAX_DELAY);
  ESP_LOGI(wifi_tag, "STOPPED");
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
      IP_EVENT, ESP_EVENT_ANY_ID, m_ip_event_handler_instance));
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
      WIFI_EVENT, ESP_EVENT_ANY_ID, m_wifi_event_handler_instance));
  vSemaphoreDelete(m_semaphore);
}

MacAddress WifiConnection::get_mac_address() {
  MacAddress mac_address;
  ESP_ERROR_CHECK(
      esp_read_mac((uint8_t *)(void *)&mac_address, ESP_MAC_WIFI_STA));

  return mac_address;
}
