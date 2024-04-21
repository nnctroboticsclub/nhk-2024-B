#pragma once

#include <cstdint>

#include <esp_gap_ble_api.h>

namespace ble::gap {

class GAP {
  static constexpr char *TAG = "GAP";

  //* Config

  char *manufacturer_ = "ESP32";
  std::uint8_t service_uuid[16] = {
      /* LSB
         <-------------------------------------------------------------------------------->
         MSB */
      // first uuid, 16bit, [12],[13] is the value
      0x88, 0xb5, 0x1f, 0x2e, 0x2c, 0x84, 0xc0, 0xa7,
      0x51, 0x41, 0x7e, 0x40, 0xc5, 0x52, 0xf6, 0x92};

  esp_ble_adv_params_t heart_rate_adv_params = {
      .adv_int_min = 0x100,
      .adv_int_max = 0x100,
      .adv_type = ADV_TYPE_IND,
      .own_addr_type = BLE_ADDR_TYPE_RPA_PUBLIC,
      .channel_map = ADV_CHNL_ALL,
      .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
  };

  // config adv data
  esp_ble_adv_data_t heart_rate_adv_config = {
      .set_scan_rsp = false,
      .include_txpower = true,
      .min_interval = 0x0006,  // slave connection min interval, Time =
                               // min_interval * 1.25 msec
      .max_interval = 0x0010,  // slave connection max interval, Time =
                               // max_interval * 1.25 msec
      .appearance = 0x00,
      .manufacturer_len = 0,        // TEST_MANUFACTURER_DATA_LEN,
      .p_manufacturer_data = NULL,  //&test_manufacturer[0],
      .service_data_len = 0,
      .p_service_data = NULL,
      .service_uuid_len = 0,   // Updated by UpdateConfigs#GAP()
      .p_service_uuid = NULL,  // Updated by UpdateConfigs#GAP()
      .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
  };

  // config scan response data
  esp_ble_adv_data_t heart_rate_scan_rsp_config = {
      .set_scan_rsp = true,
      .include_name = true,
      .manufacturer_len = 0,
      .p_manufacturer_data = NULL,
  };

  void UpdateConfigs() {
    heart_rate_scan_rsp_config.manufacturer_len = strlen(manufacturer_);
    heart_rate_scan_rsp_config.p_manufacturer_data = (uint8_t *)manufacturer_;

    heart_rate_adv_config.service_uuid_len = sizeof(service_uuid);
    heart_rate_adv_config.p_service_uuid = service_uuid;
  }

  //* Internal state
  static constexpr int kAdvConfigFlag = (1 << 0);
  static constexpr int kScanRspConfigFlag = (1 << 1);
  int adv_config_done = 0;

 public:
  void SetManufacturer(char *manufacturer) {
    manufacturer_ = manufacturer;

    UpdateConfigs();
  }

  void SetUUID(const uint8_t *uuid) {
    memcpy(service_uuid, uuid, sizeof(service_uuid));

    UpdateConfigs();
  }

  void EventHandler(esp_gap_ble_cb_event_t event,
                    esp_ble_gap_cb_param_t *param) {
    switch (event) {
      case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        ESP_LOGI(TAG, "ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT");
        adv_config_done &= (~kScanRspConfigFlag);
        if (adv_config_done == 0) {
          esp_ble_gap_start_advertising(&heart_rate_adv_params);
        }
        break;

      case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        ESP_LOGI(TAG, "ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT");
        adv_config_done &= (~kAdvConfigFlag);
        if (adv_config_done == 0) {
          esp_ble_gap_start_advertising(&heart_rate_adv_params);
        }
        break;

      case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
          ESP_LOGE(TAG, "advertising start failed, error status = %x",
                   param->adv_start_cmpl.status);
          break;
        }
        ESP_LOGI(TAG, "advertising start success");
        break;

      case ESP_GAP_BLE_AUTH_CMPL_EVT: {
        esp_bd_addr_t bd_addr;
        memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr,
               sizeof(esp_bd_addr_t));
        ESP_LOGI(TAG, "remote BD_ADDR: %08x%04x",
                 (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) +
                     bd_addr[3],
                 (bd_addr[4] << 8) + bd_addr[5]);
        ESP_LOGI(TAG, "address type = %d",
                 param->ble_security.auth_cmpl.addr_type);
        ESP_LOGI(TAG, "pair status = %s",
                 param->ble_security.auth_cmpl.success ? "success" : "fail");
        if (!param->ble_security.auth_cmpl.success) {
          ESP_LOGI(TAG, "fail reason = 0x%x",
                   param->ble_security.auth_cmpl.fail_reason);
        } else {
          ESP_LOGI(TAG, "auth mode = %d",
                   param->ble_security.auth_cmpl.auth_mode);
        }
        break;
      }
      case ESP_GAP_BLE_KEY_EVT:
        // shows the ble key info share with peer device to the user.
        ESP_LOGI(TAG, "key type = %d", param->ble_security.ble_key.key_type);
        break;
      case ESP_GAP_BLE_OOB_REQ_EVT: {
        ESP_LOGI(TAG, "ESP_GAP_BLE_OOB_REQ_EVT");
        uint8_t tk[16] = {1};
        esp_ble_oob_req_reply(param->ble_security.ble_req.bd_addr, tk,
                              sizeof(tk));
        break;
      }
      case ESP_GAP_BLE_NC_REQ_EVT: {
        esp_ble_confirm_reply(param->ble_security.ble_req.bd_addr, true);
        ESP_LOGI(TAG,
                 "ESP_GAP_BLE_NC_REQ_EVT, the passkey Notify number:%" PRIu32,
                 param->ble_security.key_notif.passkey);
        break;
      }
      case ESP_GAP_BLE_SEC_REQ_EVT: {
        ESP_LOGI(TAG, "ESP_GAP_BLE_SEC_REQ_EVT");
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
      }
      case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT: {
        ESP_LOGI(TAG, "ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT");

        if (param->local_privacy_cmpl.status != ESP_BT_STATUS_SUCCESS) {
          ESP_LOGE(TAG, "config local privacy failed, error status = %x",
                   param->local_privacy_cmpl.status);
          break;
        }

        esp_err_t ret = esp_ble_gap_config_adv_data(&heart_rate_adv_config);
        if (ret) {
          ESP_LOGE(TAG, "config adv data failed, error code = %x", ret);
        } else {
          adv_config_done |= kAdvConfigFlag;
        }

        ret = esp_ble_gap_config_adv_data(&heart_rate_scan_rsp_config);
        if (ret) {
          ESP_LOGE(TAG, "config adv data failed, error code = %x", ret);
        } else {
          adv_config_done |= kScanRspConfigFlag;
        }

        break;
      }

      default: {
        ESP_LOGE(TAG, "Unhandled GAP event: %d", event);
        break;
      }
    }
  }
};

}  // namespace ble::gap