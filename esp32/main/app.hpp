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

#include "ble/gatt/attr_db.hpp"
#include "ble/gatt/profile.hpp"
#include "ble/gatt/gatt.hpp"
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

/// Heart Rate Sensor Service
static const uint16_t heart_rate_svc = ESP_GATT_UUID_serial;

static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid =
    ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t char_prop_notify = ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_read_write =
    ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ;

/// Heart Rate Sensor Service - Heart Rate Measurement Characteristic, notify
static const uint16_t heart_rate_meas_uuid = ESP_GATT_HEART_RATE_MEAS;
static uint8_t heart_measurement_ccc[2] = {0x00, 0x00};

/// Heart Rate Sensor Service -Body Sensor Location characteristic, read
static const uint16_t body_sensor_location_uuid = ESP_GATT_BODY_SENSOR_LOCATION;
static uint8_t body_sensor_loc_val[1] = {0x00};

/// Heart Rate Sensor Service - Heart Rate Control Point characteristic,
/// write&read
static const uint16_t heart_rate_ctrl_point = ESP_GATT_HEART_RATE_CNTL_POINT;
static uint8_t heart_ctrl_point[1] = {0x00};

class Profile_CANStream : public ble::gatt::GattsProfile {
  static constexpr const char *TAG = "Profile#CAN";
  enum attrs {
    HRS_IDX_SVC,

    HRS_IDX_HR_MEAS_CHAR,
    HRS_IDX_HR_MEAS_VAL,
    HRS_IDX_HR_MEAS_NTF_CFG,

    HRS_IDX_BOBY_SENSOR_LOC_CHAR,
    HRS_IDX_BOBY_SENSOR_LOC_VAL,

    HRS_IDX_HR_CTNL_PT_CHAR,
    HRS_IDX_HR_CTNL_PT_VAL,
  };

 public:
  Profile_CANStream() {
    using enum ble::gatt::AttrDb::Perm;
    attrs_.AddService(HRS_IDX_SVC, primary_service_uuid, heart_rate_svc, kRead);
    attrs_.AddService(HRS_IDX_HR_MEAS_CHAR, character_declaration_uuid,
                      char_prop_notify, kRead);
    attrs_.AddService(HRS_IDX_HR_MEAS_VAL, heart_rate_meas_uuid, kRead);
    attrs_.AddService(HRS_IDX_HR_MEAS_NTF_CFG, character_client_config_uuid,
                      heart_measurement_ccc, kReadWrite);
    attrs_.AddService(HRS_IDX_BOBY_SENSOR_LOC_CHAR, character_declaration_uuid,
                      char_prop_read, kRead);
    attrs_.AddService(HRS_IDX_BOBY_SENSOR_LOC_VAL, body_sensor_location_uuid,
                      body_sensor_loc_val, kERead);
    attrs_.AddService(HRS_IDX_HR_CTNL_PT_CHAR, character_declaration_uuid,
                      char_prop_read_write, kRead);
    attrs_.AddService(HRS_IDX_HR_CTNL_PT_VAL, heart_rate_ctrl_point,
                      heart_ctrl_point, kEReadWrite);
  }

  void StartService() override {
    esp_ble_gatts_start_service(attrs_.GetEntry(HRS_IDX_SVC)->handle);
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

  gatt.AddProfile(std::make_unique<Profile_CANStream>());

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