#pragma once

#include <stddef.h>
#include <stdint.h>

namespace stm32_usb::host {

enum class HCStatus {
  kIdle,
  kXfrc,  // Transfer completed

  kHalted,
  kNak,
  kNYet,
  kStall,

  kXActErr,
  kBabbleErr,
  kDataToggleErr
};

enum class UrbStatus {
  kIdle,
  kDone,

  kNotReady,
  kNYet,
  kError,
  kStall
};

// Host Channel (HAL_HCD_HC)
class HC {
 private:
  class Impl;
  Impl* impl_;

 public:
  HC();
  ~HC();

  /**
   * @brief Initialize the Host Channel
   * @param ep: Endpoint number
   *            00h ~ 0Fh for Out, 80h ~ 8Fh for In
   * @param dev: Device address 0 ~ 255
   * @param ep_type: Endpoint type. can be EP_TYPE_XXX
   * @param max_packet_size: Maximum packet size
   */
  void Init(int ep, int dev, int ep_type, int max_packet_size);

  /**
   * @brief Submit a request to the Host Channel
   * @param direction: Direction of the request.
   *                   0 for OUT, 1 for IN
   * @param pbuff: Buffer to send or receive data
   * @param length: Length of the data
   * @param setup: true if the request is SETUP, false if DATA
   * @param do_ping: true if the request is PING
   */
  void SubmitRequest(int direction, uint8_t* pbuff, int length, bool setup,
                     bool do_ping);

  HCStatus GetState();
  UrbStatus GetURBState();

  int GetXferCount();

  void DataToggle(int toggle);
  int DataToggle();
};
}  // namespace stm32_usb::host
