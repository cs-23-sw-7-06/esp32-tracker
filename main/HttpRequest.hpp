#pragma once

#include <future>
#include <string>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <esp_err.h>
#include <esp_http_client.h>

class HttpRequest {
private:
  std::promise<std::string> m_promise;
  esp_http_client_handle_t m_handle;
  std::string m_post_data;
  std::string m_response_data;
  int m_retry_count = 0;

  bool m_exiting = false;
  SemaphoreHandle_t m_exit_semaphore;

  static esp_err_t http_event_handler(esp_http_client_event_t *event);

  void internal_send();

public:
  HttpRequest(const char *url, esp_http_client_method_t method);
  HttpRequest(const HttpRequest &) = delete;
  ~HttpRequest();

  std::string send();
  std::string send_with_post_data(std::string post_data);
};
