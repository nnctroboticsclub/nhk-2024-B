#include "hc.hpp"

#include "hcd.hpp"

#include <mbed.h>
#include <stm32f4xx_hal_def.h>
#include <stm32f4xx_hal_hcd.h>

#include <robotics/logger/logger.hpp>
#include <robotics/platform/timer.hpp>

namespace {
//* Logger
robotics::logger::Logger logger("hc.host.usb",
                                "\x1b[32mUSB \x1b[34mHC   \x1b[0m");

//* Logger
robotics::logger::Logger slot_logger(
    "slot.hc.host.usb", "\x1b[32mUSB \x1b[34mHC \x1b[35mS \x1b[0m");

constexpr const bool log_verbose_enabled = false;
constexpr const bool log_enabled = false;

//* Available channel mask
// Example:
//   0b0000 0000 0000 0000: All ch1 ~ ch15 are available
//   0b0000 0000 0000 0001: ch1 is not available
int slot = 0x00;

class SlotMarker {
  static int FindAvailableSlot(int start = 0) {
    int i;

    for (i = start;
         // if slot & mask is 0, then the slot is available, so break the loop
         i < 16 && (slot & (1 << i));
         // 0b0000 0000 0000 0001 -> 0b0000 0000 0000 0010
         i = (i + 1) & 0x0F);

    if (i == 14) {
      return -1;
    }

    return i + 1;
  }

  int mask_;
  int ch_;

 public:
  SlotMarker() {
    ch_ = 1;
    Next();
  }

  ~SlotMarker() {
    // slot_logger.Debug("Release channel %d", ch_);
    slot &= ~mask_;
  }

  void Next() {
    ch_ = FindAvailableSlot(ch_);
    if (ch_ == -1) {
      slot_logger.Error("No available channel");
      return;
    }

    slot &= ~mask_;    // release old channel
    mask_ = 1 << ch_;  // set new channel
    slot |= mask_;     // acquire new channel
  }

  int GetChannel() { return ch_; }
};

}  // namespace

namespace stm32_usb::host {
class HC::Impl {
  SlotMarker slot_;
  HCD_HandleTypeDef *hhcd_;

  int ep_type_;
  int ep_;
  bool data01_;

 public:
  Impl() : hhcd_(stm32_usb::HCD::GetInstance()->GetHandle()) {
    if (slot_.GetChannel() == -1) {
      return;
    }

    if (log_verbose_enabled) {
      logger.Trace("<<<<<< Acquire channel %d", slot_.GetChannel());
    }

    robotics::system::Timer timer;
    timer.Reset();
    timer.Start();

    while (Idle()) {
      if (timer.ElapsedTime() >= 1s) {
        logger.Error("Failed to acquire channel %d", slot_.GetChannel());
        logger.Error("State: %d", GetState());
        slot_.Next();
        timer.Reset();
      }
    }

    if (log_verbose_enabled) {
      logger.Trace("channel %d Ready!", slot_.GetChannel());
    }

    if (this->ep_ & 0x80) {
      data01_ = hhcd_->hc[slot_.GetChannel()].toggle_in == 1;
    } else {
      data01_ = hhcd_->hc[slot_.GetChannel()].toggle_out == 1;
    }
  }

  ~Impl() {
    hhcd_->hc[slot_.GetChannel()].state = HCD_HCStateTypeDef::HC_IDLE;
    if (log_verbose_enabled) {
      logger.Trace(">>>>>> Release channel %d", slot_.GetChannel());
    }
  }

  bool Idle() {
    return GetState() != HCD_HCStateTypeDef::HC_IDLE &&
           GetState() != HCD_HCStateTypeDef::HC_XFRC;
  }
  bool UrbIdle() { return GetURBState() != HCD_URBStateTypeDef::URB_IDLE; }

  bool DirIn() { return (ep_ & 0x80) == 0x80; }

  /**
   * @brief Initialize the Host Channel
   * @param ep: Endpoint number 1 ~ 15
   * @param dev: Device address 0 ~ 255
   * @param speed: Device speed. can be HCD_SPEED_XXX
   * @param ep_type: Endpoint type. can be EP_TYPE_XXX
   * @param max_packet_size: Maximum packet size
   */
  void Init(int ep, int dev, int speed, int ep_type, int max_packet_size) {
    ep_ = ep;
    ep_type_ = ep_type;
    if (log_verbose_enabled)
      logger.Debug("Init ch%02d dev%02x(spd:%d)-ep%02d(%d) max_packet_size:%d",
                   slot_.GetChannel(), dev, speed, ep, ep_type,
                   max_packet_size);
    auto ret = HAL_HCD_HC_Init(hhcd_, slot_.GetChannel(), ep & 0x7F, dev, speed,
                               ep_type, max_packet_size);

    if (ret != HAL_StatusTypeDef::HAL_OK) {
      logger.Error("HAL_HCD_HC_Init failed with %d", ret);
      return;
    }
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
  void SubmitRequest(int, uint8_t *pbuff, int length, bool setup,
                     bool do_ping) {
    // 0: SETUP
    // 1: DATA1
    int token = setup ? 0 : 1;

    if (log_enabled && !DirIn()) {
      logger.Debug("%s: ch%2d ep:%02x(%d) <== %#08X (t=%d, l=%d)",
                   setup ? "\x1b[1;33m--S->\x1b[0m" : "\x1b[33m---->\x1b[0m",
                   slot_.GetChannel(), ep_, ep_type_, pbuff, data01_, length);
      logger.Hex(robotics::logger::core::Level::kDebug, pbuff, length);
    }

    if (DirIn()) {
      hhcd_->hc[slot_.GetChannel()].toggle_in = data01_ ? 1 : 0;
    } else {
      hhcd_->hc[slot_.GetChannel()].toggle_out = data01_ ? 1 : 0;
    }

    auto ret =
        HAL_HCD_HC_SubmitRequest(hhcd_, slot_.GetChannel(), DirIn() ? 1 : 0,
                                 ep_type_, token, pbuff, length, do_ping);
    while (GetState() == HCD_HCStateTypeDef::HC_IDLE) {
    }

    if (ret != HAL_StatusTypeDef::HAL_OK) {
      logger.Error("HAL_HCD_HC_SubmitRequest failed with %d", ret);
      return;
    }

    if (log_enabled && DirIn()) {
      logger.Debug("%s: ch%2d ep:%02x(%d) ==> %#08X (t=%d, l=%d)",
                   setup ? "\x1b[1;34m<-S--\x1b[0m" : "\x1b[34m<----\x1b[0m",
                   slot_.GetChannel(), ep_, ep_type_, pbuff, data01_, length);
      logger.Hex(robotics::logger::core::Level::kDebug, pbuff, length);
    }
  }

  bool Data01() { return data01_; }

  void Data01(bool data01) { data01_ = data01; }

  void ToggleData01() { Data01(!Data01()); }

  HCD_HCStateTypeDef GetState() {
    return HAL_HCD_HC_GetState(hhcd_, slot_.GetChannel());
  }

  HCD_URBStateTypeDef GetURBState() {
    return HAL_HCD_HC_GetURBState(hhcd_, slot_.GetChannel());
  }

  int GetXferCount() {
    return HAL_HCD_HC_GetXferCount(hhcd_, slot_.GetChannel());
  }
};

HC::HC() : impl_(new Impl) {}

HC::~HC() { delete impl_; }

void HC::Init(int ep, int dev, int speed, int ep_type, int max_packet_size) {
  impl_->Init(ep, dev, speed, ep_type, max_packet_size);
}

void HC::SubmitRequest(int direction, uint8_t *pbuff, int length, bool setup,
                       bool do_ping) {
  impl_->SubmitRequest(direction, pbuff, length, setup, do_ping);
}

bool HC::Idle() { return impl_->Idle(); }
bool HC::UrbIdle() { return impl_->UrbIdle(); }

HCD_HCStateTypeDef HC::GetState() { return impl_->GetState(); }

HCD_URBStateTypeDef HC::GetURBState() { return impl_->GetURBState(); }

int HC::GetXferCount() { return impl_->GetXferCount(); }
bool HC::Data01() { return impl_->Data01(); }
void HC::Data01(bool data01) { impl_->Data01(data01); }
void HC::ToggleData01() { impl_->ToggleData01(); }
}  // namespace stm32_usb::host