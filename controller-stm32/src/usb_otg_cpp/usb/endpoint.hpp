#pragma once

#include <cstddef>

#include "device.hpp"

namespace stm32_usb::host {

enum class EndpointResult { kOk, kNotReady, kStalled, kError };

struct Endpoint {
  int endpoint_address_ = 0;
  const int ep_type_ = 0;  // EP_TYPE_XXX
  int max_packet_size_ = -1;
  Device& device_;
  bool data01_ = false;

  EndpointResult SendPacket_1(const uint8_t* pbuff, const int length,
                              const bool setup_packet = false,
                              const bool do_ping = false);
  EndpointResult ReceivePacket_1(uint8_t* pbuff, const int length);

 protected:
  EndpointResult SendPacket(const uint8_t* pbuff, const int length,
                            const bool setup_packet = false,
                            const bool do_ping = false);
  EndpointResult ReceivePacket(uint8_t* pbuff, const int length);

  EndpointResult SendSetupPacket(uint8_t* pbuff, int tx_length) {
    return SendPacket(pbuff, tx_length, true, true);
  }

  EndpointResult SendDataIn(uint8_t* pbuff, int rx_length) {
    return ReceivePacket(pbuff, rx_length);
  }

  EndpointResult SendDataOut(uint8_t* pbuff, int tx_length) {
    return SendPacket(pbuff, tx_length);
  }

 public:
  Endpoint(Device& device, int ep, int ep_type)
      : endpoint_address_(ep), ep_type_(ep_type), device_(device) {}
  virtual ~Endpoint() = default;

  void SetMaxPacketSize(int max_packet_size) {
    max_packet_size_ = max_packet_size;
  }

  bool Data01() { return data01_; }
  void Data01(bool data01) { data01_ = data01; }
  void ToggleData01() { Data01(!Data01()); }
};

class EndpointControl : public Endpoint {
 public:
  EndpointControl(Device& device, int ep);

  virtual ~EndpointControl() = default;

  // For Out: Request
  // For In: Response
  EndpointResult ControlRead(uint8_t* pbuff, int tx_len, const int rx_len);

  EndpointResult ControlWrite(uint8_t* setup_data, int setup_len, uint8_t* data,
                              int data_len);
};
}  // namespace stm32_usb::host