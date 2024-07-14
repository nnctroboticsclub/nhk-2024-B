#pragma once

#include <stm32f4xx_hal_def.h>
#include <stm32f4xx_hal_hcd.h>

namespace stm32_usb {

class HCD {
 public:
  HCD();
  void Init();
  void WaitForAttach();

  HCD_HandleTypeDef* GetHandle();

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