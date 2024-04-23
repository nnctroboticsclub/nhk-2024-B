#pragma once

#include "char.hpp"
#include <sstream>
#include <iomanip>

#include <esp_gap_ble_api.h>
#include <esp_log.h>

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
  std::vector<std::unique_ptr<Characteristic>> characteristics;

  void AddCharacteristic(Characteristic *characteristic) {
    characteristics.push_back(std::unique_ptr<Characteristic>(characteristic));
  }

  void SetServiceUUID(uint16_t uuid) { service_header_.SetValue(uuid); }

  void SetServiceUUID(const uint8_t (&uuid)[16]) {
    service_header_.SetValue(uuid, 16);
  }

 public:
  Attribute service_header_;
  std::vector<Attribute *> attributes_;

  void CollectAttributes(std::vector<Attribute *> &attributes) {
    service_header_.CollectAttributes(attributes);

    for (auto &characteristic : characteristics) {
      characteristic->CollectAttributes(attributes);
    }
  }

 public:
  bool initialized = false;
  virtual void Init() = 0;

 public:
  Service() : service_header_() {
    service_header_.SetUUID(internal::primary_service_uuid)
        .SetPermissions(Attribute::Perm::kRead);
  }

  Service(const Service &) = delete;
  Service &operator=(const Service &) = delete;

  void RegisterAttributeTable(int service_id, esp_gatt_if_t gatt_if) {
    ESP_LOGI(TAG, "Collecting attributes for service %d", service_id);
    ESP_LOGI(TAG, "Characteristics %d", characteristics.size());
    attributes_.clear();
    CollectAttributes(attributes_);

    // convert to vector of esp_gatts_attr_db_t
    std::vector<esp_gatts_attr_db_t> esp_attributes;
    for (auto &attribute : attributes_) {
      esp_attributes.push_back(*attribute);
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

    esp_ble_gatts_create_attr_tab(esp_attributes.data(), gatt_if,
                                  esp_attributes.size(), service_id);
  }

  void LoadHandles(
      esp_ble_gatts_cb_param_t::gatts_add_attr_tab_evt_param *param) {
    for (int i = 0; i < param->num_handle; i++) {
      this->attributes_[i]->SetHandle(param->handles[i]);
    }
  }

  bool OnWrite(uint16_t handle, std::vector<uint8_t> value, uint16_t offset) {
    for (auto &characteristic : characteristics) {
      auto &value_attr = characteristic->GetValueAttr();

      if (value_attr.Write(handle, value, offset)) {
        return true;
      }
    }

    return false;
  }
};

}  // namespace ble::gatt