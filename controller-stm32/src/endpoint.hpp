#pragma once

#include <cstddef>

#include "device.hpp"

namespace stm32_usb::host {
struct Endpoint {
  int endpoint_address_ = 0;
  const int ep_type_ = 0;  // EP_TYPE_XXX
  Device& device_;

 protected:
  int SendPacket(const uint8_t* pbuff, const int length,
                 const bool setup_packet);
  HCD_URBStateTypeDef ReceivePacket(uint8_t* pbuff, const int length);

 public:
  Endpoint(Device& device, int ep, int ep_type)
      : device_(device), endpoint_address_(ep), ep_type_(ep_type) {}
  virtual ~Endpoint() = default;
};

class EndpointControl : public Endpoint {
 public:
  EndpointControl(Device& device, int ep)
      : Endpoint(device, ep, EP_TYPE_CTRL) {}
  virtual ~EndpointControl() = default;

  void ControlRead(uint8_t* pbuff, int length);
  void ControlWrite(uint8_t* pbuff, int length);
};
}  // namespace stm32_usb::host