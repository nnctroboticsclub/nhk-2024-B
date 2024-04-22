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
#include "ble/gatt/profile.hpp"
#include "ble/gap.hpp"

/*
 * DEFINES
 ****************************************************************************************
 */

#define HRPS_HT_MEAS_MAX_LEN (13)

#define HRPS_MANDATORY_MASK (0x0F)
#define HRPS_BODY_SENSOR_LOC_MASK (0x30)
#define HRPS_HR_CTNL_PT_MASK (0xC0)

/// Attributes State Machine
#define GATTS_TABLE_TAG "SEC_GATTS_DEMO"

#define ESP_HEART_RATE_APP_ID 0x55
#define EXAMPLE_DEVICE_NAME "ESP_BLE_SECURITY"

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40

/*
 *  Heart Rate PROFILE ATTRIBUTES
 ****************************************************************************************
 */

/// Full HRS Database Description - Used to add attributes into the database

static void show_bonded_devices(void) {
  int dev_num = esp_ble_get_bond_device_num();
  if (dev_num == 0) {
    ESP_LOGI(GATTS_TABLE_TAG, "Bonded devices number zero\n");
    return;
  }

  esp_ble_bond_dev_t *dev_list =
      (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
  if (!dev_list) {
    ESP_LOGI(GATTS_TABLE_TAG, "malloc failed, return\n");
    return;
  }
  esp_ble_get_bond_device_list(&dev_num, dev_list);
  ESP_LOGI(GATTS_TABLE_TAG, "Bonded devices number : %d", dev_num);

  ESP_LOGI(GATTS_TABLE_TAG, "Bonded devices list : %d", dev_num);
  for (int i = 0; i < dev_num; i++) {
    esp_log_buffer_hex(GATTS_TABLE_TAG, (void *)dev_list[i].bd_addr,
                       sizeof(esp_bd_addr_t));
  }

  free(dev_list);
}

static void __attribute__((unused)) remove_all_bonded_devices(void) {
  int dev_num = esp_ble_get_bond_device_num();
  if (dev_num == 0) {
    ESP_LOGI(GATTS_TABLE_TAG, "Bonded devices number zero\n");
    return;
  }

  esp_ble_bond_dev_t *dev_list =
      (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
  if (!dev_list) {
    ESP_LOGI(GATTS_TABLE_TAG, "malloc failed, return\n");
    return;
  }
  esp_ble_get_bond_device_list(&dev_num, dev_list);
  for (int i = 0; i < dev_num; i++) {
    esp_ble_remove_bond_device(dev_list[i].bd_addr);
  }

  free(dev_list);
}

namespace ble_can {

const uint8_t service_uuid[16] = {0x7e, 0x5c, 0xfd, 0x53, 0x1f, 0xe5,
                                  0x82, 0x91, 0x19, 0x4d, 0xda, 0x51,
                                  0x3e, 0xf2, 0xb6, 0x20};

const uint8_t bus_load_uuid[16] = {0x4c, 0x1f, 0x5e, 0x79, 0xb7, 0x5b,
                                   0x7e, 0xbc, 0x7b, 0x46, 0x7e, 0xcb,
                                   0x41, 0x1d, 0xef, 0x59};
uint16_t bus_load_val = 0;

struct bus_packet {
  uint8_t op;
  uint8_t len;
  uint8_t data[8];
};

const uint8_t bus_rx_uuid[16] = {0xe8, 0x29, 0xe1, 0x89, 0x53, 0x74,
                                 0x04, 0xa2, 0xfb, 0x4e, 0x3a, 0x61,
                                 0xb2, 0x87, 0xff, 0x3e};
bus_packet bus_rx_val;

const uint8_t bus_tx_uuid[16] = {0x4e, 0x9d, 0x73, 0xd5, 0x17, 0x62,
                                 0xda, 0x81, 0x04, 0x4c, 0x1a, 0xd6,
                                 0xf8, 0x7f, 0x07, 0x18};
bus_packet bus_tx_val;

class CANStreamingService : public ble::gatt::Service {
  static constexpr const char *TAG = "Profile#CAN";
  enum attrs {
    kIdxSvc,
  };

 public:
  CANStreamingService() {}

  void Init() override {
    this->SetServiceUUID(service_uuid);

    this->AddCharacteristic(
        &(new ble::gatt::Characteristic())
             ->SetUUID(bus_load_uuid)
             .SetValue(bus_load_val)
             .SetPermissions(ble::gatt::Attribute::Perm::kRead)
             .SetProperties(ble::gatt::chararacteristic::Prop::kNotify)  //
             .SetMaxValueLength(2)
        /* .AddDescriptor(&(new ble::gatt::Attribute())
                            ->SetValue((uint8_t &)heart_measurement_ccc)
                            .SetUUID(ESP_GATT_UUID_CHAR_CLIENT_CONFIG)
                            .SetMaxValueLength(2)
                            .SetPermissions(
                                ble::gatt::Attribute::Perm::kReadWrite)  //
                       )                                                 // */
    );

    this->AddCharacteristic(
        &(new ble::gatt::Characteristic())
             ->SetUUID(bus_rx_uuid)
             .SetPermissions(ble::gatt::Attribute::Perm::kERead)       //
             .SetProperties(ble::gatt::chararacteristic::Prop::kRead)  //
             .SetValue(bus_rx_val)                                     //
    );

    this->AddCharacteristic(
        &(new ble::gatt::Characteristic())
             ->SetUUID(bus_tx_uuid)
             .SetPermissions(ble::gatt::Attribute::Perm::kEReadWrite)     //
             .SetProperties(ble::gatt::chararacteristic::Prop::kWriteNR)  //
             .SetValue(bus_rx_val)                                        //
    );
  }

  void Callback(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                esp_ble_gatts_cb_param_t *param) override {
    ESP_LOGV(TAG, "event = %x", event);
    switch (event) {
      case ESP_GATTS_WRITE_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_WRITE_EVT, write value:");
        esp_log_buffer_hex(TAG, param->write.value, param->write.len);
        break;

      default:
        ESP_LOGE(TAG, "Unhandled event: %d", event);
        break;
    }
  }
};
}  // namespace ble_can

ble::gap::GAP gap;
ble::gatt::GATT gatt;

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

  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

  gatt.SetDeviceName(EXAMPLE_DEVICE_NAME);

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ret = esp_bt_controller_init(&bt_cfg);
  if (ret) {
    ESP_LOGE(GATTS_TABLE_TAG, "%s init controller failed: %s", __func__,
             esp_err_to_name(ret));
    return;
  }
  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  if (ret) {
    ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s", __func__,
             esp_err_to_name(ret));
    return;
  }

  ESP_LOGI(GATTS_TABLE_TAG, "%s init bluetooth", __func__);
  esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
  ret = esp_bluedroid_init_with_cfg(&bluedroid_cfg);
  if (ret) {
    ESP_LOGE(GATTS_TABLE_TAG, "%s init bluetooth failed: %s", __func__,
             esp_err_to_name(ret));
    return;
  }
  ret = esp_bluedroid_enable();
  if (ret) {
    ESP_LOGE(GATTS_TABLE_TAG, "%s enable bluetooth failed: %s", __func__,
             esp_err_to_name(ret));
    return;
  }

  gatt.AddProfile(std::make_unique<ble_can::CANStreamingService>());

  ret = esp_ble_gatts_register_callback([](esp_gatts_cb_event_t event,
                                           esp_gatt_if_t gatts_if,
                                           esp_ble_gatts_cb_param_t *param) {
    gatt.gatts_event_handler(event, gatts_if, param);
  });
  if (ret) {
    ESP_LOGE(GATTS_TABLE_TAG, "gatts register error, error code = %x", ret);
    return;
  }
  ret = esp_ble_gap_register_callback(
      [](esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
        gap.EventHandler(event, param);
      });
  if (ret) {
    ESP_LOGE(GATTS_TABLE_TAG, "gap register error, error code = %x", ret);
    return;
  }
  ret = esp_ble_gatts_app_register(ESP_HEART_RATE_APP_ID);
  if (ret) {
    ESP_LOGE(GATTS_TABLE_TAG, "gatts app register error, error code = %x", ret);
    return;
  }

  /* set the security iocap & auth_req & key size & init key response key
   * parameters to the stack*/
  esp_ble_auth_req_t auth_req =
      ESP_LE_AUTH_REQ_SC_MITM_BOND;  // bonding with peer device after
                                     // authentication
  esp_ble_io_cap_t iocap =
      ESP_IO_CAP_NONE;    // set the IO capability to No output No input
  uint8_t key_size = 16;  // the key size should be 7~16 bytes
  uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  // set static passkey
  uint32_t passkey = 123456;
  uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
  uint8_t oob_support = ESP_BLE_OOB_DISABLE;
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey,
                                 sizeof(uint32_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req,
                                 sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap,
                                 sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size,
                                 sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH,
                                 &auth_option, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support,
                                 sizeof(uint8_t));
  /* If your BLE device acts as a Slave, the init_key means you hope which types
  of key of the master should distribute to you, and the response key means
  which key you can distribute to the master; If your BLE device acts as a
  master, the response key means you hope which types of key of the slave should
  distribute to you, and the init key means which key you can distribute to the
  slave. */
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key,
                                 sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key,
                                 sizeof(uint8_t));

  /* Just show how to clear all the bonded devices
   * Delay 30s, clear all the bonded devices
   *
   * vTaskDelay(30000 / portTICK_PERIOD_MS);
   * remove_all_bonded_devices();
   */
}