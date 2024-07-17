#include "endpoint.hpp"

#include <usb/hc.hpp>

#include <robotics/logger/logger.hpp>

#include <mbed.h>
#include <stm32f4xx_hal_def.h>
#include <stm32f4xx_hal_hcd.h>

namespace {
robotics::logger::Logger logger("ep.host.usb",
                                "\x1b[32mUSB \x1b[35mEP   \x1b[0m");
}

namespace stm32_usb::host {

EndpointResult Endpoint::SendPacket_1(const uint8_t* pbuff, const int length,
                                      const bool setup_packet,
                                      const bool do_ping) {
  HC hc;
  hc.Init(endpoint_address_, device_.GetAddress(), device_.GetSpeed(), ep_type_,
          max_packet_size_ == -1 ? 32 : max_packet_size_);

  hc.Data01(data01_);
  hc.SubmitRequest(0, const_cast<uint8_t*>(pbuff), length, setup_packet,
                   do_ping);

  while (!hc.UrbIdle());
  auto state = hc.GetURBState();
  if (state != UrbStatus::kDone) {
    if (state == UrbStatus::kNotReady) {
      return EndpointResult::kNotReady;
    }
    logger.Error("Send failed; %d", state);
    return EndpointResult::kError;
  }

  ToggleData01();

  return EndpointResult::kOk;
}

EndpointResult Endpoint::ReceivePacket_1(uint8_t* pbuff, const int length) {
  HC hc;
  hc.Init(endpoint_address_ | 0x80, device_.GetAddress(), device_.GetSpeed(),
          ep_type_, max_packet_size_ == -1 ? 32 : max_packet_size_);

  hc.Data01(data01_);
  hc.SubmitRequest(1, pbuff, length, false, false);

  while (!hc.UrbIdle());

  auto state = hc.GetURBState();
  if (state != UrbStatus::kDone) {
    if (state == UrbStatus::kNotReady) {
      return EndpointResult::kNotReady;
    }
    logger.Error("Send failed; %d", state);
    return EndpointResult::kError;
  }

  ToggleData01();
  return EndpointResult::kOk;
}

EndpointResult Endpoint::ReceivePacket(uint8_t* pbuff, const int length) {
  int rx_chunks = length / max_packet_size_;
  int rx_remain = length % max_packet_size_;

  // logger.Debug("ReceivePacket: %d --> x%d +%d", length, rx_chunks,
  // rx_remain);
  for (int i = 0; i < rx_chunks; i++) {
    auto result =
        ReceivePacket_1(pbuff + i * max_packet_size_, max_packet_size_);
    if (result != EndpointResult::kOk) {
      logger.Error("ReceivePacket failed (Chunk%d; %d)", i, result);
      return result;
    }
  }

  if (rx_remain > 0) {
    auto result =
        ReceivePacket_1(pbuff + rx_chunks * max_packet_size_, rx_remain);
    if (result != EndpointResult::kOk) {
      logger.Error("ReceivePacket failed (Remain; %d)", result);
      return result;
    }
  }

  return EndpointResult::kOk;
}

EndpointResult Endpoint::SendPacket(const uint8_t* pbuff, const int length,
                                    const bool setup_packet,
                                    const bool do_ping) {
  int tx_chunks = length / max_packet_size_;
  int tx_remain = length % max_packet_size_;
  // logger.Trace("SendPacket: %d --> x%d +%d", length, tx_chunks, tx_remain);

  for (int i = 0; i < tx_chunks; i++) {
    auto result = SendPacket_1(pbuff + i * max_packet_size_, max_packet_size_,
                               setup_packet, do_ping);
    if (result != EndpointResult::kOk) {
      logger.Error("SendPacket failed (Chunk%d; %d)", i, result);
      return result;
    }
  }

  if (tx_remain > 0) {
    auto result = SendPacket_1(pbuff + tx_chunks * max_packet_size_, tx_remain,
                               setup_packet, do_ping);
    if (result != EndpointResult::kOk) {
      logger.Error("SendPacket failed (Remain; %d)", result);
      return result;
    }
  }

  return EndpointResult::kOk;
}

EndpointResult EndpointControl::ControlRead(uint8_t* pbuff, int tx_length,
                                            int rx_length) {
  // O SETUP
  // I DATA
  // O DATA

  // logger.Debug("ControlRead: %d -> %d", tx_length, rx_length);

  //* SETUP
  Data01(false);
  {
    auto result = SendPacket(pbuff, tx_length, true, true);
    if (result != EndpointResult::kOk) {
      logger.Error("ControlRead failed (Setup Stage; %d)", result);
      return result;
    }
  }

  //* DATA
  Data01(true);
  {
    auto result = ReceivePacket(pbuff, rx_length);
    if (result != EndpointResult::kOk) {
      logger.Error("ControlRead failed (Data Stage; %d)", result);
      return result;
    }
  }

  //* STATUS
  Data01(true);
  {
    auto result = SendPacket(NULL, 0);
    if (result != EndpointResult::kOk) {
      logger.Error("ControlRead failed (result Stage; %d)", result);
      return result;
    }
  }

  return EndpointResult::kOk;
}

EndpointResult EndpointControl::ControlWrite(uint8_t* setup_data, int setup_len,
                                             uint8_t* data, int data_len) {
  // O SETUP
  // I DATA
  // O DATA

  //* SETUP
  SendPacket(setup_data, setup_len, true, true);

  //* DATA
  SendPacket(data, data_len);

  //* DATA
  ReceivePacket(nullptr, 0);

  return EndpointResult::kOk;
}

EndpointControl::EndpointControl(Device& device, int ep)
    : Endpoint(device, ep, EP_TYPE_CTRL) {}
}  // namespace stm32_usb::host