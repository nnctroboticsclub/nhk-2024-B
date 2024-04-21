#pragma once

#include "attr_db.hpp"

#define HEART_RATE_SVC_INST_ID 0

namespace ble::gatt {
static esp_ble_adv_params_t heart_rate_adv_params = {
    .adv_int_min = 0x100,
    .adv_int_max = 0x100,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_RPA_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};
class GattsProfile {
  static constexpr const char *TAG = "Profile$";

  uint8_t service_id_;

 protected:
  ble::gatt::AttrDb attrs_;

  virtual void Callback(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                        esp_ble_gatts_cb_param_t *param) = 0;

  virtual void StartService() = 0;

 public:
  esp_gatts_cb_t gatts_cb;
  uint16_t gatts_if;

  void SetServiceId(uint8_t service_id) { service_id_ = service_id; }

  void CallCallback(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                    esp_ble_gatts_cb_param_t *param) {
    switch (event) {
      case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "REGISTER_APP_EVT, status %d, app_id %d",
                 param->reg.status, param->reg.app_id);

        esp_ble_gatts_create_attr_tab(attrs_.GetDb().data(), gatts_if,
                                      attrs_.GetDb().size(), service_id_);
        break;

      case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT");
        esp_ble_set_encryption(param->connect.remote_bda,
                               ESP_BLE_SEC_ENCRYPT_MITM);
        break;

      case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x",
                 param->disconnect.reason);
        esp_ble_gap_start_advertising(&heart_rate_adv_params);
        break;

      case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
        ESP_LOGI(TAG, "The number handle = %x", param->add_attr_tab.num_handle);
        if (param->create.status == ESP_GATT_OK) {
          if (param->add_attr_tab.num_handle == attrs_.GetDb().size()) {
            attrs_.LoadHandles(param->add_attr_tab.handles,
                               param->add_attr_tab.num_handle);
            StartService();
          } else {
            ESP_LOGE(
                TAG,
                "Create attribute table abnormally, num_handle (%d) doesn't "
                "equal to attrs_.GetDb().size()(%d)",
                param->add_attr_tab.num_handle, attrs_.GetDb().size());
          }
        } else {
          ESP_LOGE(TAG, " Create attribute table failed, error code = %x",
                   param->create.status);
        }
        break;
      }

      default:
        Callback(event, gatts_if, param);
        break;
    }
  }
};

}  // namespace ble::gatt