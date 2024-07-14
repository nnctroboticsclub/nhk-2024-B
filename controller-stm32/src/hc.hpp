#pragma once

namespace stm32_usb::host {
// Host Channel (HAL_HCD_HC)
class HC {
 private:
  class Impl;
  Impl* impl_;

 public:
  HC();
  /**
   * @brief Initialize the Host Channel
   * @param ep: Endpoint number 1 ~ 15
   * @param dev: Device address 0 ~ 255
   * @param speed: Device speed. can be HCD_SPEED_XXX
   * @param ep_type: Endpoint type. can be EP_TYPE_XXX
   * @param max_packet_size: Maximum packet size
   */
  void Init(int ep, int dev, int speed, int ep_type, int max_packet_size);

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

  HCD_HCStateTypeDef GetState();
  HCD_URBStateTypeDef GetURB();
};
}  // namespace stm32_usb::host