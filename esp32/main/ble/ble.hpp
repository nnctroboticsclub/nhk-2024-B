#pragma once

#include <string>

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"

#include "gap.hpp"
#include "gatt/gatt.hpp"

namespace ble {
class BLE {
  static constexpr const char *TAG = "BLE";

 public:
  gap::GAP gap;
  gatt::GATT gatt;

 private:
  std::string device_name;

  esp_err_t InitBT() {
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    auto ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
      ESP_LOGE(TAG, "%s init controller failed: %s", __func__,
               esp_err_to_name(ret));
      return ret;
    }
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
      ESP_LOGE(TAG, "%s enable controller failed: %s", __func__,
               esp_err_to_name(ret));
      return ret;
    }

    return ESP_OK;
  }

  esp_err_t InitBlueDroid() {
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    auto ret = esp_bluedroid_init_with_cfg(&bluedroid_cfg);
    if (ret) {
      ESP_LOGE(TAG, "%s init bluetooth failed: %s", __func__,
               esp_err_to_name(ret));
      return ret;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
      ESP_LOGE(TAG, "%s enable bluetooth failed: %s", __func__,
               esp_err_to_name(ret));
      return ret;
    }

    return ESP_OK;
  }

 public:
  void Init() {
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    gatt.SetDeviceName("CAN Debugger");

    if (InitBT() != ESP_OK) {
      return;
    }

    if (InitBlueDroid() != ESP_OK) {
      return;
    }

    auto ret = esp_ble_gatts_register_callback(
        [](esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
           esp_ble_gatts_cb_param_t *param) {
          BLE::GetInstance()->gatt.EventHandler(event, gatts_if, param);
        });
    if (ret) {
      ESP_LOGE(TAG, "gatts register error, error code = %x", ret);
      return;
    }
    ret = esp_ble_gap_register_callback(
        [](esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
          BLE::GetInstance()->gap.EventHandler(event, param);
        });
    if (ret) {
      ESP_LOGE(TAG, "gap register error, error code = %x", ret);
      return;
    }
    ret = esp_ble_gatts_app_register(0);
    if (ret) {
      ESP_LOGE(TAG, "gatts app register error, error code = %x", ret);
      return;
    }

    /* set the security iocap & auth_req & key size & init key response key
     * parameters to the stack*/
    gap.InitSecurity();
  }

 private:
  static BLE *instance;
  BLE() {}

 public:
  static BLE *GetInstance() {
    if (!instance) {
      instance = new BLE();
    }
    return instance;
  }
};

BLE *BLE::instance = nullptr;
}  // namespace ble