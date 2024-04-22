#pragma once

#include <variant>
#include <vector>
#include <esp_gatts_api.h>

namespace ble::gatt {
class Attribute : public esp_gatts_attr_db_t {
 public:
  enum class Perm : uint16_t {
    kRead = ESP_GATT_PERM_READ,
    kERead = ESP_GATT_PERM_READ_ENCRYPTED,
    kWrite = ESP_GATT_PERM_WRITE,
    kReadWrite = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
    kEReadWrite = ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED,
  };

  uint16_t handle;
  uint16_t uuid_16_;

 public:
  Attribute()
      : esp_gatts_attr_db_t{
            .attr_control =
                {
                    .auto_rsp = ESP_GATT_AUTO_RSP,
                },
            .att_desc =
                {
                    .uuid_length = 0,
                    .uuid_p = nullptr,
                    .perm = 0,
                    .max_length = 0,
                    .length = 0,
                    .value = nullptr,
                },
        }, handle(0), uuid_16_(0) {
    ESP_LOGI("BLE:GATT:Attr", "Created Attribute at %p", this);
  }

  Attribute &SetUUID(const uint16_t &uuid) {
    this->uuid_16_ = uuid;
    ESP_LOGI("BLE:GATT:Attr", "Setting UUID: %04X -> %p (this=%p)", uuid_16_,
             &uuid_16_, this);
    att_desc.uuid_length = 2;
    att_desc.uuid_p = reinterpret_cast<uint8_t *>(&uuid_16_);
    return *this;
  }

  Attribute &SetUUID(const uint8_t uuid[16]) {
    ESP_LOGI("BLE:GATT:Attr", "Setting UUID(128)");
    att_desc.uuid_length = 16;
    att_desc.uuid_p = (uint8_t *)uuid;
    return *this;
  }

  Attribute &SetPermissions(Perm permissions) {
    att_desc.perm = (uint8_t)permissions;
    return *this;
  }

  template <typename T>
  Attribute &SetValue(T &value) {
    att_desc.max_length = sizeof(value);
    att_desc.length = sizeof(value);
    att_desc.value = reinterpret_cast<uint8_t *>(&value);
    return *this;
  }

  Attribute &SetMaxValueLength(uint16_t length) {
    att_desc.max_length = length;
    return *this;
  }

  void CollectAttributes(std::vector<Attribute> &db) { db.push_back(*this); }
};
}  // namespace ble::gatt