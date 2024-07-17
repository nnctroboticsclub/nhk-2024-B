#pragma once

namespace stm32_usb {

/*
 * USB_OTG_FS GPIO Configuration
 * PA11: USB_OTG_FS_DM
 * PA12: USB_OTG_FS_DP
 */
class HCD {
 public:
  HCD();
  ~HCD();

  void Init();
  void WaitForAttach();

  void* GetHandle();  // HCD_HandleTypeDef

 private:
  static HCD* instance;

 public:
  static HCD* GetInstance() {
    if (!instance) {
      instance = new HCD;
    }
    return instance;
  }
};

}  // namespace stm32_usb