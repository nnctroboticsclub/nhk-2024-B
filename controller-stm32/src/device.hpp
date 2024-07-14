#pragma once

#include <cstdint>

#include "hc.hpp"

namespace stm32_usb::host {
class Device {
  bool is_enumarated_ = false;

  uint8_t address_ = 0;

 public:
  Device() {}

  void SetAddress(uint8_t address) { address_ = address; }
  uint8_t GetAddress() { return address_; }

  void SetEnumerated(bool is_enumarated) { is_enumarated_ = is_enumarated; }
  bool IsEnumerated() { return is_enumarated_; }

  int GetSpeed() { return HCD_SPEED_FULL; }

  void Reset() {
    is_enumarated_ = false;
    address_ = 0;
  }
};
}  // namespace stm32_usb::host