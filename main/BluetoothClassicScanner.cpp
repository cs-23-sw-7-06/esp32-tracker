#include "BluetoothClassicScanner.hpp"

#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_err.h>
#include <esp_gap_ble_api.h>
#include <esp_gattc_api.h>
#include <esp_log.h>

static constexpr auto bt_tag = "bt-scan";

BluetoothClassicScanner *BluetoothClassicScanner::m_instance;

void BluetoothClassicScanner::gap_callback(esp_bt_gap_cb_event_t event,
                                           esp_bt_gap_cb_param_t *param) {
  switch (event) {
  case ESP_BT_GAP_DISC_RES_EVT: {
    const auto mac_address = create_mac_address(
        param->disc_res.bda[0], param->disc_res.bda[1], param->disc_res.bda[2],
        param->disc_res.bda[3], param->disc_res.bda[4], param->disc_res.bda[5]);

    int8_t rssi = INT8_MAX;

    for (int i = 0; i < param->disc_res.num_prop; i++) {
      switch (param->disc_res.prop[i].type) {
      case ESP_BT_GAP_DEV_PROP_BDNAME:
        ESP_LOGV(bt_tag, "ESP_BT_GAP_DEV_PROP_BDNAME");
        ESP_LOGV(bt_tag, "Name: %.*s", param->disc_res.prop[i].len,
                 (const char *)param->disc_res.prop[i].val);
        break;
      case ESP_BT_GAP_DEV_PROP_COD:
        ESP_LOGV(bt_tag, "ESP_BT_GAP_DEV_PROP_COD");
        break;
      case ESP_BT_GAP_DEV_PROP_RSSI:
        ESP_LOGV(bt_tag, "ESP_BT_GAP_DEV_PROP_RSSI");
        rssi = *(uint8_t *)param->disc_res.prop[i].val;
        break;

      case ESP_BT_GAP_DEV_PROP_EIR:
        ESP_LOGV(bt_tag, "ESP_BT_GAP_DEV_PROP_EIR");
        break;
      }
    }

    LocalizationTarget localization_target(mac_address, SnifferType::Bluetooth,
                                           rssi);

    m_instance->m_localization_targets.push_back(localization_target);
  }
    return;
  case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
    ESP_LOGI(bt_tag, "ESP_BT_GAP_DISC_STATE_CHANGED_EVT");
    switch (param->disc_st_chg.state) {
    case ESP_BT_GAP_DISCOVERY_STOPPED:
      xSemaphoreGive(m_instance->m_semaphore);
      break;
    case ESP_BT_GAP_DISCOVERY_STARTED:
      break;
    }
    return;
  case ESP_BT_GAP_RMT_SRVCS_EVT:
    ESP_LOGI(bt_tag, "ESP_BT_GAP_RMT_SRVCS_EVT");
    return;
  case ESP_BT_GAP_RMT_SRVC_REC_EVT:
    ESP_LOGI(bt_tag, "ESP_BT_GAP_RMT_SRVC_REC_EVT");
    return;
  case ESP_BT_GAP_AUTH_CMPL_EVT:
    ESP_LOGI(bt_tag, "ESP_BT_GAP_AUTH_CMPL_EVT");
    return;
  case ESP_BT_GAP_PIN_REQ_EVT:
    ESP_LOGI(bt_tag, "ESP_BT_GAP_PIN_REQ_EVT");
    return;
  case ESP_BT_GAP_CFM_REQ_EVT:
    ESP_LOGI(bt_tag, "ESP_BT_GAP_CFM_REQ_EVT");
    return;
  case ESP_BT_GAP_KEY_NOTIF_EVT:
    ESP_LOGI(bt_tag, "ESP_BT_GAP_NOTIF_KEY");
    return;
  case ESP_BT_GAP_KEY_REQ_EVT:
    ESP_LOGI(bt_tag, "ESP_BT_GAP_KEY_REQ_EVT");
    return;
  case ESP_BT_GAP_READ_RSSI_DELTA_EVT:
    ESP_LOGI(bt_tag, "ESP_BT_GAP_READ_RSSI_DELTA_EVT");
    return;
  case ESP_BT_GAP_CONFIG_EIR_DATA_EVT:
    ESP_LOGI(bt_tag, "ESP_BT_GAP_CONFIG_EIR_DATA_EVT");
    return;
  case ESP_BT_GAP_SET_AFH_CHANNELS_EVT:
    ESP_LOGI(bt_tag, "ESP_BT_GAP_SET_AFH_CHANNELS_EVT");
    return;
  case ESP_BT_GAP_READ_REMOTE_NAME_EVT:
    ESP_LOGI(bt_tag, "ESP_BT_GAP_READ_REMOTE_NAME_EVT");
    return;
  case ESP_BT_GAP_MODE_CHG_EVT:
    ESP_LOGI(bt_tag, "ESP_BT_GAP_MODE_CHG_EVT");
    return;
  case ESP_BT_GAP_REMOVE_BOND_DEV_COMPLETE_EVT:
    ESP_LOGI(bt_tag, "ESP_BT_GAP_REMOVE_BOND_DEV_COMPLETE_EVT");
    return;
  case ESP_BT_GAP_QOS_CMPL_EVT:
    ESP_LOGI(bt_tag, "ESP_BT_GAP_QOS_CMPL_EVT");
    return;
  case ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
    ESP_LOGI(bt_tag, "ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT");
    return;
  case ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:
    ESP_LOGI(bt_tag, "ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT");
    return;
  case ESP_BT_GAP_EVT_MAX:
    ESP_LOGE(bt_tag, "ESP_BT_GAP_EVT_MAX");
    return;

  default:
    ESP_LOGE(bt_tag, "ESP_BT_UNKNOWN!");
    return;
  }
}

std::vector<LocalizationTarget> BluetoothClassicScanner::scan(int timeout_ms) {
  ESP_LOGI(bt_tag, "STARTING BT SCAN");
  m_localization_targets = std::vector<LocalizationTarget>();
  const auto timeout = static_cast<float>(timeout_ms) * 1.28f / 1000.0f;
  ESP_ERROR_CHECK(
      esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, timeout, 0));

  // Wait for discovery to complete
  xSemaphoreTake(m_semaphore, portMAX_DELAY);

  ESP_LOGI(bt_tag, "FINISHED BT SCAN");
  return m_localization_targets;
}

BluetoothClassicScanner::BluetoothClassicScanner() {
  ESP_LOGI(bt_tag, "STARTING BT");
  m_instance = this;
  m_semaphore = xSemaphoreCreateCounting(1, 0);
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
  ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BTDM));
  ESP_ERROR_CHECK(esp_bluedroid_init());
  ESP_ERROR_CHECK(esp_bluedroid_enable());
  ESP_ERROR_CHECK(esp_bt_gap_register_callback(gap_callback));
}

BluetoothClassicScanner::~BluetoothClassicScanner() {
  ESP_LOGI(bt_tag, "STOPPING BT");
  ESP_ERROR_CHECK(esp_bluedroid_disable());
  ESP_ERROR_CHECK(esp_bluedroid_deinit());
  ESP_ERROR_CHECK(esp_bt_controller_disable());
  ESP_ERROR_CHECK(esp_bt_controller_deinit());
  vSemaphoreDelete(m_semaphore);
  m_instance = nullptr;
}
