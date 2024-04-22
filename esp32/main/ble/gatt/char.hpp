#pragma once

#include "attr.hpp"

#include <cstdint>

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
 public:
  Attribute declare;
  Attribute value;

  std::vector<Attribute> descriptors;

 public:
  Characteristic() : declare(), value(), descriptors() {
    declare.SetUUID(internal::char_declare_uuid)
        .SetPermissions(Attribute::Perm::kRead)
        .SetMaxValueLength(2);

    ESP_LOGI("BLE:GATT:Char", "Created Characteristic at %p", this);
  }

  Characteristic& SetProperties(Prop properties) {
    declare.SetValue(PropToRef(properties)).SetMaxValueLength(2);

    return *this;
  }

  Characteristic& SetUUID(uint16_t uuid) {
    ESP_LOGI("BLE:GATT:Char", "Setting UUID: %4x -> %p (this=%p)", uuid, &value,
             this);
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
    descriptors.push_back(*desc);

    return *this;
  }

  void CollectAttributes(std::vector<Attribute>& db) {
    declare.CollectAttributes(db);

    value.CollectAttributes(db);

    for (auto& desc : descriptors) {
      desc.CollectAttributes(db);
    }
  }
};
}  // namespace chararacteristic

using Characteristic = chararacteristic::Characteristic;
}  // namespace ble::gatt