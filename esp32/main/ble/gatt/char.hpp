#pragma once

#include "attr.hpp"

#include <cstdint>
#include <algorithm>

#include <vector>

namespace ble::gatt {
namespace chararacteristic {
namespace internal {
uint8_t kBroadcast = ESP_GATT_CHAR_PROP_BIT_BROADCAST;
uint8_t kRead = ESP_GATT_CHAR_PROP_BIT_READ;
uint8_t kWriteNR = ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
uint8_t kWrite = ESP_GATT_CHAR_PROP_BIT_WRITE;
uint8_t kNotify = ESP_GATT_CHAR_PROP_BIT_NOTIFY;
uint8_t kIndicate = ESP_GATT_CHAR_PROP_BIT_INDICATE;

uint8_t kRW = kRead | kWrite;
uint8_t kRWNR = kRead | kWriteNR;

uint16_t char_declare_uuid = ESP_GATT_UUID_CHAR_DECLARE;
}  // namespace internal

enum Prop : std::uint8_t {
  kBroadcast,
  kRead,
  kWriteNR,
  kWrite,
  kNotify,
  kIndicate,

  kRW,
  kRWNR,
};

std::uint8_t& PropToRef(Prop prop) {
  switch (prop) {
    case kBroadcast:
      return internal::kBroadcast;
    case kRead:
      return internal::kRead;
    case kWriteNR:
      return internal::kWriteNR;
    case kWrite:
      return internal::kWrite;
    case kNotify:
      return internal::kNotify;
    case kIndicate:
      return internal::kIndicate;

    case kRW:
      return internal::kRW;
    case kRWNR:
      return internal::kRWNR;

    default:
      return internal::kRW;
  }
}

class Characteristic {
  Attribute declare;
  Attribute value;

  std::vector<std::unique_ptr<Attribute>> descriptors;
  std::vector<esp_gatt_if_t> interfaces;

 public:
  void IndicateValue() {}

 public:
  Characteristic() : declare(), value(), descriptors() {
    declare.SetUUID(internal::char_declare_uuid)
        .SetPermissions(Attribute::Perm::kRead)
        .SetMaxValueLength(2);
  }

  Characteristic(const Characteristic&) = delete;
  Characteristic& operator=(const Characteristic&) = delete;

  Characteristic& SetProperties(Prop properties) {
    declare.SetValue(PropToRef(properties)).SetMaxValueLength(2);

    return *this;
  }

  Characteristic& SetUUID(uint16_t uuid) {
    value.SetUUID(uuid);

    return *this;
  }

  Characteristic& SetUUID(const uint8_t uuid[16]) {
    value.SetUUID(uuid);

    return *this;
  }

  Characteristic& SetPermissions(Attribute::Perm permissions) {
    value.SetPermissions(permissions);

    return *this;
  }

  Characteristic& SetMaxValueLength(uint16_t length) {
    value.SetMaxValueLength(length);

    return *this;
  }

  template <typename T>
  Characteristic& SetValue(T& value) {
    this->value.SetValue(value);

    return *this;
  }

  Characteristic& AddDescriptor(Attribute* desc) {
    descriptors.push_back(std::unique_ptr<Attribute>(desc));

    return *this;
  }

  bool Write(uint16_t dest_handle, std::vector<uint8_t>& value,
             uint16_t offset = 0) {
    return this->value.Write(dest_handle, value, offset);
  }

  Attribute& GetValueAttr() { return value; }

  Characteristic& SetOnWriteCallback(Attribute::OnWriteCallback callback) {
    value.SetOnWriteCallback(callback);

    return *this;
  }

  void CollectAttributes(std::vector<Attribute*>& db) {
    declare.CollectAttributes(db);

    value.CollectAttributes(db);

    for (auto& desc : descriptors) {
      desc->CollectAttributes(db);
    }
  }
};
}  // namespace chararacteristic

using Characteristic = chararacteristic::Characteristic;
}  // namespace ble::gatt