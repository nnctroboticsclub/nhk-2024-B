#pragma once

#include "char.hpp"
#include <sstream>
#include <iomanip>

#define HEART_RATE_SVC_INST_ID 0

namespace ble::gatt {
namespace internal {
uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
}
static esp_ble_adv_params_t heart_rate_adv_params = {
    .adv_int_min = 0x100,
    .adv_int_max = 0x100,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_RPA_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

class Service {
  static constexpr const char *TAG = "Profile$";

 protected:  //* Protected members
  std::vector<Characteristic> characteristics;

  void AddCharacteristic(Characteristic *characteristic) {
    characteristics.push_back(*characteristic);
  }

  virtual void Callback(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                        esp_ble_gatts_cb_param_t *param) = 0;

 private:
  uint8_t service_id_;

  Attribute service_header_;

  std::vector<Attribute> attributes;
  std::vector<esp_gatts_attr_db_t> esp_attributes;

  void CollectAttributes(std::vector<Attribute> &attributes) {
    UpdateSettings();

    service_header_.CollectAttributes(attributes);

    for (auto &characteristic : characteristics) {
      characteristic.CollectAttributes(attributes);
    }
  }

  void UpdateSettings() {
    service_header_.SetUUID(internal::primary_service_uuid)
        .SetPermissions(Attribute::Perm::kRead);
  }

 public:
  bool initialized = false;
  virtual void Init() = 0;

 public:
  esp_gatts_cb_t gatts_cb;
  uint16_t gatts_if;

  void SetServiceId(uint8_t service_id) { service_id_ = service_id; }

  void SetServiceUUID(uint16_t uuid) { service_header_.SetValue(uuid); }

  void SetServiceUUID(const uint8_t (&uuid)[16]) {
    service_header_.SetValue(uuid, 16);
  }

  void CallCallback(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                    esp_ble_gatts_cb_param_t *param) {
    switch (event) {
      case ESP_GATTS_REG_EVT: {
        ESP_LOGI(TAG, "REGISTER_APP_EVT, status %d, app_id %d",
                 param->reg.status, param->reg.app_id);

        attributes.clear();
        CollectAttributes(attributes);

        // convert to vector of esp_gatts_attr_db_t
        esp_attributes.clear();
        for (auto &attribute : attributes) {
          esp_attributes.push_back(attribute);
        }

        ESP_LOGI(TAG, "Creating attribute table with %d esp-attributes",
                 esp_attributes.size());

        for (int i = 0; i < esp_attributes.size(); i++) {
          std::stringstream ss;
          ss << "Attribute " << i << ": ";
          ss << std::setw(4) << std::setfill('0') << std::hex
             << esp_attributes[i].att_desc.perm << " ";
          ss << std::setw(4) << std::setfill('0') << std::hex
             << esp_attributes[i].att_desc.max_length << ", ";

          for (int j = 0; j < esp_attributes[i].att_desc.uuid_length; j++) {
            ss << std::setw(2) << std::setfill('0') << std::hex
               << (int)esp_attributes[i].att_desc.uuid_p[j];
          }
          ss << ", ";

          for (int j = 0; j < esp_attributes[i].att_desc.length; j++) {
            ss << std::setw(2) << std::setfill('0') << std::hex
               << (int)esp_attributes[i].att_desc.value[j];
          }

          ESP_LOGI(TAG, "%s", ss.str().c_str());
        }

        esp_ble_gatts_create_attr_tab(esp_attributes.data(), gatts_if,
                                      esp_attributes.size(), service_id_);
        break;
      }

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
          if (param->add_attr_tab.num_handle == attributes.size()) {
            ESP_LOGI(TAG,
                     "Create attribute table successfully, the number "
                     "handle = %d\n",
                     param->add_attr_tab.num_handle);
            esp_ble_gatts_start_service(param->add_attr_tab.handles[0]);
          } else {
            ESP_LOGE(
                TAG,
                "Create attribute table abnormally, num_handle (%d) doesn't "
                "equal to attrs_.GetDb().size()(%d)",
                param->add_attr_tab.num_handle, attributes.size());
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