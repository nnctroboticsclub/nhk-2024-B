#include "hc.hpp"

#include "hcd.hpp"

#include <mbed.h>
#include <stm32f4xx_hal_def.h>
#include <stm32f4xx_hal_hcd.h>

#include <robotics/logger/logger.hpp>
#include <robotics/platform/timer.hpp>
#include <robotics/platform/thread.hpp>
#include <robotics/platform/random.hpp>

namespace {
//* Logger
robotics::logger::Logger logger("hc.host.usb",
                                "\x1b[32mUSB \x1b[34mHC   \x1b[0m");

//* Logger
robotics::logger::Logger slot_logger(
    "slot.hc.host.usb", "\x1b[32mUSB \x1b[34mHC \x1b[35mS \x1b[0m");

constexpr const bool log_verbose_enabled = false;
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
/* extern "C" void HAL_HCD_HC_NotifyURBChange_Callback(
    HCD_HandleTypeDef *hhcd, uint8_t chnum, HCD_URBStateTypeDef urb_state) {
  logger.Debug("HC%02d s%d, u%d x%d", chnum, hhcd->hc[chnum].state, urb_state,
               hhcd->hc[chnum].xfer_count);
} */

namespace stm32_usb::host {
class HC::Impl {
  SlotMarker slot_;
  HCD_HandleTypeDef *hhcd_;
  HCD_HCTypeDef *hc_ = nullptr;

  robotics::system::Thread thread_;

  bool thread_stop_request_mark_ = false;
  bool thread_running_mark_ = false;

  int ep_type_;
  int toggle_ = 0;

 public:
  Impl()
      : hhcd_((HCD_HandleTypeDef *)stm32_usb::HCD::GetInstance()->GetHandle()) {
    if (slot_.GetChannel() == -1) {
      return;
    }

    hc_ = &hhcd_->hc[slot_.GetChannel()];

    if (log_verbose_enabled) {
      logger.Trace("<<<<<< Acquire channel %d", slot_.GetChannel());
    }
  }

  ~Impl() {
    thread_stop_request_mark_ = true;
    while (thread_running_mark_) robotics::system::SleepFor(10ms);

    if (log_verbose_enabled)
      logger.Trace(">>>>>> Release channel %d", slot_.GetChannel());

    hc_->state = HC_IDLE;
    hc_->urb_state = URB_IDLE;
  }

  /**
   * @brief Initialize the Host Channel
   * @param ep_addr: Endpoint number 1 ~ 15
   * @param dev: Device address 0 ~ 255
   * @param speed: Device speed. can be HCD_SPEED_XXX
   * @param ep_type: Endpoint type. can be EP_TYPE_XXX
   * @param max_packet_size: Maximum packet size
   */
  void Init(int ep_addr, int dev_addr, int ep_type, int max_packet_size) {
    ep_type_ = ep_type;

    auto speed = HAL_HCD_GetCurrentSpeed(hhcd_) == HCD_SPEED_LOW
                     ? HCD_SPEED_LOW
                     : HCD_SPEED_FULL;

    hc_->dev_addr = dev_addr;
    hc_->ch_num = slot_.GetChannel();
    hc_->ep_num = ep_addr;

    hc_->speed = speed;
    // hc_->do_ping
    // hc_->process_ping
    hc_->ep_type = ep_type;
    hc_->max_packet = max_packet_size;

    logger.Info("Init HC[%d]: ep%d, dev%d, speed%d, type%d, size%d",
                slot_.GetChannel(), ep_addr, dev_addr, speed, ep_type,
                max_packet_size);

    auto ret = USB_HC_Init(hhcd_->Instance, slot_.GetChannel(), ep_addr,
                           dev_addr, speed, ep_type, max_packet_size);

    if (ret != HAL_StatusTypeDef::HAL_OK) {
      logger.Error("HAL_HCD_HC_Init failed with %d", ret);
      return;
    }

    if (0)
      thread_.Start([this]() {
        volatile auto &urb_status = hc_->urb_state;
        volatile auto &hc_status = hc_->state;

        thread_running_mark_ = true;

        int thread_id = robotics::system::Random::GetByte() / 255.f * 10;

        auto state_urb = (HCD_URBStateTypeDef)-1;
        auto state_hc = (HCD_HCStateTypeDef)-1;

        int tick = 0;

        while (!thread_stop_request_mark_) {
          if (urb_status != state_urb || hc_status != state_hc
              //  ||tick % 5000000 == 0
          ) {
            state_urb = urb_status;
            state_hc = hc_status;

            logger.Debug("HC[%d]: u%d h%d, XferCount %d",
                         slot_.GetChannel(),  //
                         (int)state_urb,      //
                         (int)state_hc,       //
                         GetXferCount());
            // tick = 0;
          }

          // tick += 1;
        }
        thread_running_mark_ = false;
      });
    robotics::system::SleepFor(10ms);
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
    hc_->ep_is_in = direction;

    hc_->data_pid = setup          ? HC_PID_SETUP
                    : toggle_ == 0 ? HC_PID_DATA0
                    : toggle_ == 1 ? HC_PID_DATA1
                    : toggle_ == 2 ? HC_PID_DATA2
                                   : HC_PID_DATA0;

    hc_->xfer_buff = pbuff;
    hc_->xfer_len = length;
    hc_->xfer_count = 0;

    /* logger.Info("data_pid%d, toggle[in,out]=[%d,%d] dir%d ping%d",
                hc_->data_pid, hc_->toggle_in, hc_->toggle_out, direction,
                do_ping); */

    auto ret = USB_HC_StartXfer(hhcd_->Instance, hc_, hhcd_->Init.dma_enable);

    if (ret != HAL_StatusTypeDef::HAL_OK) {
      logger.Error("HAL_HCD_HC_SubmitRequest failed with %d", ret);
      return;
    }
  }

  HCStatus GetState() {
    auto hal_status = HAL_HCD_HC_GetState(hhcd_, slot_.GetChannel());
    switch (hal_status) {
      case HCD_HCStateTypeDef::HC_IDLE:
        return HCStatus::kIdle;

      case HCD_HCStateTypeDef::HC_XFRC:
        return HCStatus::kXfrc;
      case HCD_HCStateTypeDef::HC_HALTED:
        return HCStatus::kHalted;
      case HCD_HCStateTypeDef::HC_NAK:
        return HCStatus::kNak;
      case HCD_HCStateTypeDef::HC_NYET:
        return HCStatus::kNYet;
      case HCD_HCStateTypeDef::HC_STALL:
        return HCStatus::kStall;

      case HCD_HCStateTypeDef::HC_XACTERR:
        return HCStatus::kXActErr;

      case HCD_HCStateTypeDef::HC_BBLERR:
        return HCStatus::kBabbleErr;

      case HCD_HCStateTypeDef::HC_DATATGLERR:
        return HCStatus::kDataToggleErr;

      default:
        return HCStatus::kIdle;
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

  void DataToggle(int toggle) { toggle_ = toggle; }

  int DataToggle() { return toggle_; }
};

HC::HC() : impl_(new Impl) {}

HC::~HC() { delete impl_; }

void HC::Init(int ep, int dev, int ep_type, int max_packet_size) {
  impl_->Init(ep, dev, ep_type, max_packet_size);
}

void HC::SubmitRequest(int direction, uint8_t *pbuff, int length, bool setup,
                       bool do_ping) {
  impl_->SubmitRequest(direction, pbuff, length, setup, do_ping);
}

HCStatus HC::GetState() { return impl_->GetState(); }
UrbStatus HC::GetURBState() { return impl_->GetURBState(); }

int HC::GetXferCount() { return impl_->GetXferCount(); }

void HC::DataToggle(int toggle) { impl_->DataToggle(toggle); }

int HC::DataToggle() { return impl_->DataToggle(); }
}  // namespace stm32_usb::host