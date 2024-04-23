#pragma once

#include <variant>
#include <vector>
#include <functional>
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

  using OnWriteCallback =
      std::function<void(std::vector<uint8_t> data, uint16_t offset)>;

  uint16_t uuid_16_;
  uint16_t handle_ = -1;
  OnWriteCallback on_write_;

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
        }, uuid_16_(0) {
  }

  Attribute(const Attribute &) = delete;
  Attribute &operator=(const Attribute &) = delete;

  Attribute &SetUUID(const uint16_t uuid) {
    this->uuid_16_ = uuid;
    att_desc.uuid_length = 2;
    att_desc.uuid_p = reinterpret_cast<uint8_t *>(&uuid_16_);
    return *this;
  }

  Attribute &SetUUID(const uint8_t uuid[16]) {
    att_desc.uuid_length = 16;
    att_desc.uuid_p = (uint8_t *)uuid;
    return *this;
  }

  Attribute &SetPermissions(Perm permissions) {
    att_desc.perm = (uint8_t)permissions;
    return *this;
  }

  template <typename T>
  Attribute &SetValue(T &value, size_t size = sizeof(T)) {
    if (att_desc.value) {
      delete att_desc.value;
      att_desc.value = nullptr;
    }

    att_desc.max_length = size;
    att_desc.length = size;
    att_desc.value = new uint8_t[size];

    memcpy(att_desc.value, &value, size);
    return *this;
  }

  Attribute &SetMaxValueLength(uint16_t length) {
    att_desc.max_length = length;
    return *this;
  }

  Attribute &SetHandle(uint16_t handle) {
    ESP_LOGI("Attribute", "SetHandle: %d to %p", handle, this);
    handle_ = handle;
    return *this;
  }

  uint16_t GetHandle() { return handle_; }

  Attribute &SetOnWriteCallback(OnWriteCallback callback) {
    on_write_ = callback;
    return *this;
  }

  bool Write(uint16_t dest_handle, std::vector<uint8_t> &value,
             uint16_t offset = 0) {
    if (dest_handle != handle_) {
      return false;
    }

    if (on_write_) {
      on_write_(value, offset);
    }

    if (att_desc.value) {
      delete att_desc.value;
      att_desc.value = nullptr;
    }

    att_desc.length = value.size();
    att_desc.value = new uint8_t[value.size()];
    memcpy(att_desc.value, value.data(), value.size());

    return true;
  }

  void CollectAttributes(std::vector<Attribute *> &db) { db.push_back(this); }
};
}  // namespace ble::gatt