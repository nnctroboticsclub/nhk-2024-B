#include "hc.hpp"

#include "hcd.hpp"

#include <mbed.h>
#include <stm32f4xx_hal_hcd.h>
#include <robotics/logger/logger.hpp>

namespace {
//* Logger
robotics::logger::Logger logger("hc.host.usb",
                                "\x1b[32mUSB    \x1b[34mHC\x1b[0m");

//* Available channel mask
// Example:
//   0b0000 0000 0000 0000: All ch0 ~ ch15 are available
//   0b0000 0000 0000 0001: ch0 is not available
int slot = 0x00;

class SlotMarker {
  static int FindAvailableSlot() {
    int mask = 1;
    int i;
    for (i = 0;
         // if slot & mask is 0, then the slot is available, so break the loop
         i < 16 && (slot & mask);
         // 0b0000 0000 0000 0001 -> 0b0000 0000 0000 0010
         i++, mask <<= 1);

    if (i == 16) {
      return -1;
    }

    return i;
  }

  int mask;
  int ch;

 public:
  SlotMarker() {
    ch = FindAvailableSlot();
    if (ch == -1) {
      logger.Error("No available channel");
      return;
    }
    mask = 1 << ch;
    slot |= mask;
  }

  ~SlotMarker() { slot &= ~mask; }

  int GetChannel() { return ch; }
};
}  // namespace

namespace stm32_usb::host {
class HC::Impl {
  SlotMarker slot_;
  HCD_HandleTypeDef *hhcd_;

  int ep_type_;

 public:
  Impl() : hhcd_(stm32_usb::HCD::GetInstance()->GetHandle()) {
    if (slot_.GetChannel() == -1) {
      return;
    }
  }

  /**
   * @brief Initialize the Host Channel
   * @param ep: Endpoint number 1 ~ 15
   * @param dev: Device address 0 ~ 255
   * @param speed: Device speed. can be HCD_SPEED_XXX
   * @param ep_type: Endpoint type. can be EP_TYPE_XXX
   * @param max_packet_size: Maximum packet size
   */
  void Init(int ep, int dev, int speed, int ep_type, int max_packet_size) {
    ep_type_ = ep_type;
    HAL_HCD_HC_Init(hhcd_, slot_.GetChannel(), ep, dev, speed, ep_type,
                    max_packet_size);
  }

  /**
   * @brief Submit a request to the Host Channel
   * @param direction: Direction of the request.
   *                   0 for OUT, 1 for IN
   * @param pbuff: Buffer to send or receive data
   * @param length: Length of the data
   * @param setup: true if the request is SETUP, false if DATA
   * @param do_ping: true if the request is PING
   */
  void SubmitRequest(int direction, uint8_t *pbuff, int length, bool setup,
                     bool do_ping) {
    int token = setup ? HC_PID_SETUP : HC_PID_DATA1;  // SETUP, DATA1
    HAL_HCD_HC_SubmitRequest(hhcd_, slot_.GetChannel(), direction, ep_type_,
                             token, pbuff, length, do_ping);
  }

  HCD_HCStateTypeDef GetState() {
    return HAL_HCD_HC_GetState(hhcd_, slot_.GetChannel());
  }

  HCD_URBStateTypeDef GetURB() {
    return HAL_HCD_HC_GetURBState(hhcd_, slot_.GetChannel());
  }
};

HC::HC() : impl_(new Impl) {}

void HC::Init(int ep, int dev, int speed, int ep_type, int max_packet_size) {
  impl_->Init(ep, dev, speed, ep_type, max_packet_size);
}

void HC::SubmitRequest(int direction, uint8_t *pbuff, int length, bool setup,
                       bool do_ping) {
  impl_->SubmitRequest(direction, pbuff, length, setup, do_ping);
}

HCD_HCStateTypeDef HC::GetState() { return impl_->GetState(); }

HCD_URBStateTypeDef HC::GetURB() { return impl_->GetURB(); }
}  // namespace stm32_usb::host