/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>
#include <memory>

#include "ble/gatt/gatt.hpp"
#include "ble/gatt/service.hpp"
#include "can_streaming.hpp"
#include "ble/gap.hpp"

#include <idf-robotics/can_driver.hpp>

/*
 * DEFINES
 ****************************************************************************************
 */

/// Attributes State Machine

class BLE {
  static constexpr const char *TAG = "BLE";
  ble::gap::GAP gap;
  ble::gatt::GATT gatt;

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

    gatt.AddService(std::make_unique<ble::services::CANStreamingService>());

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

constexpr char *TAG = "CAN Debugger";

extern "C" void app_main(void) {
  esp_err_t ret;

  // Initialize NVS.
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  auto ble = BLE::GetInstance();

  ble->Init();

  ble::services::can_driver =
      std::make_shared<robotics::network::CANDriver>(GPIO_NUM_15, GPIO_NUM_4);
  ble::services::can_driver->Init();

  /* Just show how to clear all the bonded devices
   * Delay 30s, clear all the bonded devices
   *
   * vTaskDelay(30000 / portTICK_PERIOD_MS);
   * remove_all_bonded_devices();
   */
}