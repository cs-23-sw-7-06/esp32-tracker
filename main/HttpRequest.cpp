#include "HttpRequest.hpp"

#include <utility>

#include <esp_log.h>

constexpr static auto http_tag = "http";

HttpRequest::HttpRequest(const char *url, esp_http_client_method_t method) {
  esp_http_client_config_t config = {};
  config.url = url;
  config.event_handler = http_event_handler;
  config.method = method;
  config.user_data = this;
  config.user_agent = "esp32-tracker";

  m_handle = esp_http_client_init(&config);
  ESP_ERROR_CHECK(
      esp_http_client_set_header(m_handle, "Content-Type", "application/json"));
}

HttpRequest::~HttpRequest() {
  ESP_ERROR_CHECK(esp_http_client_set_user_data(m_handle, nullptr));
  ESP_ERROR_CHECK(esp_http_client_cleanup(m_handle));
}

void HttpRequest::internal_send() {
  ESP_ERROR_CHECK(esp_http_client_perform(m_handle));
}

std::string HttpRequest::send() {
  m_promise = std::promise<std::string>();
  m_response_data = std::string{};
  m_retry_count = 0;
  m_post_data = std::string{};
  ESP_LOGI(http_tag, "Sending stuff");
  ESP_ERROR_CHECK(esp_http_client_set_post_field(m_handle, nullptr, 0));
  internal_send();
  return m_promise.get_future().get();
}

std::string HttpRequest::send_with_post_data(std::string post_data) {
  m_promise = std::promise<std::string>();
  m_response_data = std::string{};
  m_retry_count = 0;
  m_post_data = post_data;
  ESP_LOGI(http_tag, "Sending stuff with post data");
  ESP_ERROR_CHECK(esp_http_client_set_post_field(m_handle, m_post_data.data(),
                                                 m_post_data.size()));
  internal_send();
  return m_promise.get_future().get();
}

esp_err_t HttpRequest::http_event_handler(esp_http_client_event_t *event) {
  if (!event->user_data) {
    ESP_LOGE(http_tag, "GOT INTO http_event_handler with destroyed user data");
  }

  auto instance = (HttpRequest *)event->user_data;
  switch (event->event_id) {
  case HTTP_EVENT_ERROR:
    ESP_LOGI(http_tag, "HTTP_EVENT_ERROR");
    if (instance->m_retry_count >= 3) {
      ESP_LOGE(http_tag, "Could not connect!");
      instance->m_promise.set_value("Error!");
      return 0;
    }
    instance->m_retry_count++;
    ESP_LOGI(http_tag, "Retry count: %d", instance->m_retry_count);
    instance->internal_send();
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGI(http_tag, "HTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGI(http_tag, "HTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_REDIRECT:
    ESP_LOGI(http_tag, "HTTP_EVENT_REDIRECT");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGI(http_tag, "HTTP_EVENT_ON_HEADER");
    printf("%s: %s\n", event->header_key, event->header_value);
    break;
  case HTTP_EVENT_ON_DATA:
    ESP_LOGI(http_tag, "HTTP_EVENT_ON_DATA, len=%d", event->data_len);
    instance->m_response_data +=
        std::string_view{(const char *)event->data, (size_t)event->data_len};
    break;
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGI(http_tag, "HTTP_EVENT_ON_FINISH");
    instance->m_promise.set_value(instance->m_response_data);
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGI(http_tag, "HTTP_EVENT_DISCONNECTED");
    break;
  }
  return ESP_OK;
}
