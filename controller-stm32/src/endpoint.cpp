#include "endpoint.hpp"

#include "hc.hpp"

#include <robotics/logger/logger.hpp>

namespace {
robotics::logger::Logger logger("ep.host.usb",
                                "\x1b[32mUSB \x1b[35mEP   \x1b[0m");
}

namespace stm32_usb::host {

int Endpoint::SendPacket(const uint8_t* pbuff, const int length,
                         const bool setup_packet) {
  HC hc;
  hc.Init(endpoint_address_, device_.GetAddress(), device_.GetSpeed(), ep_type_,
          length * 2 + length / 2);
  hc.SubmitRequest(0, const_cast<uint8_t*>(pbuff), length, setup_packet, false);

  logger.Debug("Wait for NOT URB_IDLE");
  while (hc.GetURBState() == URB_IDLE);

  logger.Debug("Send ok");
  return hc.GetXferCount();
}

HCD_URBStateTypeDef Endpoint::ReceivePacket(uint8_t* pbuff, const int length) {
  HC hc;
  hc.Init(endpoint_address_, device_.GetAddress(), device_.GetSpeed(), ep_type_,
          length * 2 + length / 2);
  hc.SubmitRequest(1, pbuff, length, false, false);

  auto state = hc.GetURBState();
  return state;
}

void EndpointControl::ControlRead(uint8_t* pbuff, int length) {
  // O SETUP
  // I DATA
  // O DATA

  //* SETUP
  {
    auto result = SendPacket(pbuff, length, true);
    if (result < 0) {
      logger.Error("SendPacket failed (Setup Stage; %d)", result);
      return;
    }
  }

  //* DATA
  {
    auto result = ReceivePacket(pbuff, length);
    if (result != HCD_URBStateTypeDef::URB_DONE) {
      logger.Error("SendPacket failed (Data Stage; %d)", result);
      return;
    }
  }

  //* DATA
  {
    auto result = SendPacket(NULL, 0, false);
    if (result < 0) {
      logger.Error("SendPacket failed (result Stage; %d)", result);
      return;
    }
  }
}

void EndpointControl::ControlWrite(uint8_t* pbuff, int length) {
  // O SETUP
  // I DATA
  // O DATA

  //* SETUP
  SendPacket(pbuff, length, true);

  //* DATA
  SendPacket(pbuff, length, false);

  //* DATA
  ReceivePacket(pbuff, length);
}
}  // namespace stm32_usb::host