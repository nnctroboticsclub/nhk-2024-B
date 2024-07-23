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

constexpr const bool log_verbose_enabled = true;
constexpr const bool log_enabled = true;

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
  HCD_HCTypeDef *hc_;

  uint8_t dev_addr_;
  uint8_t ep_addr_;

  uint8_t speed_;
  uint8_t data_toggle_;
  uint8_t max_packet_size_;
  int ep_type_;

  bool DirIn() { return (ep_addr_ & 0x80) == 0x80; }

 public:
  Impl()
      : hhcd_((HCD_HandleTypeDef *)stm32_usb::HCD::GetInstance()->GetHandle()),
        hc_(nullptr) {
    if (slot_.GetChannel() == -1) {
      return;
    }

    if (log_verbose_enabled) {
      logger.Trace("<<<<<< Acquire channel %d", slot_.GetChannel());
    }

    hc_ = &hhcd_->hc[slot_.GetChannel()];
  }

  ~Impl() {
    hc_->state = HCD_HCStateTypeDef::HC_IDLE;
    if (log_verbose_enabled) {
      logger.Trace(">>>>>> Release channel %d", slot_.GetChannel());
    }
  }

  /**
   * @brief Initialize the Host Channel
   * @param ep_addr: Endpoint number 1 ~ 15
   * @param dev: Device address 0 ~ 255
   * @param speed: Device speed. can be HCD_SPEED_XXX
   * @param ep_type: Endpoint type. can be EP_TYPE_XXX
   * @param max_packet_size: Maximum packet size
   */
  void Init(int ep_addr, int dev_addr, int speed, int ep_type,
            int max_packet_size) {
    ep_addr_ = ep_addr;
    dev_addr_ = dev_addr;
    speed_ = speed;
    ep_type_ = ep_type;
    max_packet_size_ = max_packet_size;

    auto ret = HAL_HCD_HC_Init(hhcd_, slot_.GetChannel(), ep_addr_, dev_addr,
                               speed, ep_type, max_packet_size);

    if (ret != HAL_StatusTypeDef::HAL_OK) {
      logger.Error("HAL_HCD_HC_Init failed with %d", ret);
      return;
    }
  }

  /**
   * @brief Submit a request to the Host Channel
   * @param direction: Direction of the request. 0 for OUT, 1 for IN
   * @param pbuff: Buffer to send or receive data
   * @param length: Length of the data
   * @param setup: true if the request is SETUP, false if DATA
   * @param do_ping: true if the request is PING
   */
  void SubmitRequest(int direction, uint8_t *pbuff, int length, bool setup,
                     bool do_ping) {
    // 0: SETUP
    // 1: DATA1
    int token = setup ? 0 : 1;

    if (direction) {
      hc_->toggle_in = data_toggle_;
    } else {
      hc_->toggle_out = data_toggle_;
    }

    if (log_enabled && !direction) {
      logger.Debug(
          "%s: ch%02d d%02Xe%02x (spd%d type%d) <== %#08X (t=%d, l=%d, mps=%d)",
          setup ? "\x1b[1;33m--S->\x1b[0m" : "\x1b[33m---->\x1b[0m",
          slot_.GetChannel(),                     //
          dev_addr_, ep_addr_, speed_, ep_type_,  //
          pbuff, data_toggle_, length, max_packet_size_);
      logger.Hex(robotics::logger::core::Level::kDebug, pbuff, length);
    }

    auto ret =
        HAL_HCD_HC_SubmitRequest(hhcd_, slot_.GetChannel(), direction, ep_type_,
                                 token, pbuff, length, do_ping ? 1 : 0);
    if (ret != HAL_StatusTypeDef::HAL_OK) {
      logger.Error("HAL_HCD_HC_SubmitRequest failed with %d", ret);
      return;
    }

    if (log_enabled && direction) {
      logger.Debug(
          "%s: ch%02d d%02Xe%02x (spd%d type%d) <== %#08X (t=%d, l=%d, mps=%d)",
          setup ? "\x1b[1;34m<-S--\x1b[0m" : "\x1b[34m<----\x1b[0m",
          slot_.GetChannel(),                     //
          dev_addr_, ep_addr_, speed_, ep_type_,  //
          pbuff, data_toggle_, length, max_packet_size_);
      logger.Hex(robotics::logger::core::Level::kDebug, pbuff, length);
    }
  }

  int DataToggle() { return data_toggle_; }

  void DataToggle(int toggle) { data_toggle_ = toggle; }

  HCStatus GetState() {
    auto hal_status = HAL_HCD_HC_GetState(hhcd_, slot_.GetChannel());
    switch (hal_status) {
      case HCD_HCStateTypeDef::HC_IDLE:
        return HCStatus::kIdle;

      case HCD_HCStateTypeDef::HC_XFRC:
        return HCStatus::kDone;

      case HCD_HCStateTypeDef::HC_XACTERR:
        return HCStatus::kXActErr;

      case HCD_HCStateTypeDef::HC_BBLERR:
        return HCStatus::kBabbleErr;

      case HCD_HCStateTypeDef::HC_DATATGLERR:
        return HCStatus::kDataToggleErr;

      default:
        return (GetURBState() == UrbStatus::kDone ||
                GetURBState() == UrbStatus::kIdle)
                   ? HCStatus::kIdle
                   : HCStatus::kUrbFailed;
    }
  }

  UrbStatus GetURBState() {
    auto state = HAL_HCD_HC_GetURBState(hhcd_, slot_.GetChannel());
    switch (state) {
      case HCD_URBStateTypeDef::URB_IDLE:
        return UrbStatus::kIdle;

      case HCD_URBStateTypeDef::URB_DONE:
        return UrbStatus::kDone;

      case HCD_URBStateTypeDef::URB_NOTREADY:
        return UrbStatus::kNotReady;

      case HCD_URBStateTypeDef::URB_NYET:
        return UrbStatus::kNYet;

      case HCD_URBStateTypeDef::URB_ERROR:
        return UrbStatus::kError;

      case HCD_URBStateTypeDef::URB_STALL:
        return UrbStatus::kStall;

      default:
        return UrbStatus::kIdle;
    }
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

HCStatus HC::GetState() { return impl_->GetState(); }
UrbStatus HC::GetURBState() { return impl_->GetURBState(); }

int HC::GetXferCount() { return impl_->GetXferCount(); }
bool HC::Data01() { return impl_->DataToggle(); }
void HC::Data01(bool data01) { impl_->DataToggle(data01); }
}  // namespace stm32_usb::host