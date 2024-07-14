#include "hcd.hpp"

#include <mbed.h>
#include <stm32f4xx_hal_hcd.h>
#include <robotics/logger/logger.hpp>

namespace {
//* Any USB device is connected to the USB port
volatile bool attach_done = false;

//* HCD Driver
HCD_HandleTypeDef hhcd_;

//* Logger
robotics::logger::Logger logger("hcd.host.usb",
                                "\x1b[32mUSB   \x1b[33mHCD\x1b[0m");

extern "C" void HAL_HCD_MspInit(HCD_HandleTypeDef* hhcd) {
  if (hhcd->Instance == USB_OTG_FS) {
    //* Enable clock
    __USB_OTG_FS_CLK_ENABLE();

    //* GPIO
    GPIO_InitTypeDef gpio;
    gpio.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_LOW;
    gpio.Alternate = GPIO_AF10_OTG_FS;
    HAL_GPIO_Init(GPIOA, &gpio);

    //* NVIC
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_0);
    HAL_NVIC_SetPriority(OTG_FS_IRQn, 0, 0);
    NVIC_EnableIRQ(OTG_FS_IRQn);
  }
}

void WaitForAttach() {
  using namespace std::chrono_literals;
  Timer t;
  t.reset();
  t.start();
  while (!attach_done) {
    if (t.elapsed_time() > 5s) {
      t.reset();
      logger.Info("Please attach USB device.");
    }
    ThisThread::sleep_for(10ms);
  }
}

bool IsLowSpeedHCD() {
  return HAL_HCD_GetCurrentSpeed(&hhcd_) == HCD_SPEED_LOW;
}

class HcdImpl {
 public:
  void CallIRQHandler_() { HAL_HCD_IRQHandler(&hhcd_); }

  void Attached_() {
    logger.Info("USB Device Attached");
    attach_done = true;
  }

  void Init() {
    hhcd_.Instance = USB_OTG_FS;
    hhcd_.Init.Host_channels = 8;
    hhcd_.Init.speed = HCD_SPEED_FULL;
    hhcd_.Init.dma_enable = DISABLE;
    hhcd_.Init.phy_itface = HCD_PHY_EMBEDDED;
    hhcd_.Init.Sof_enable = DISABLE;
    hhcd_.Init.low_power_enable = ENABLE;
    hhcd_.Init.vbus_sensing_enable = DISABLE;
    hhcd_.Init.use_external_vbus = DISABLE;

    auto ret = HAL_HCD_Init(&hhcd_);
    if (ret != HAL_StatusTypeDef::HAL_OK) {
      logger.Error("HAL_HCD_Init failed with %d", ret);
      return;
    }
    ::HAL_HCD_MspInit(&hhcd_);

    hhcd_.pData = this;

    ret = HAL_HCD_Start(&hhcd_);
    if (ret != HAL_StatusTypeDef::HAL_OK) {
      logger.Error("HAL_HCD_Init failed with %d", ret);
      return;
    }
  }
  void WaitAttach() {
    ::WaitForAttach();

    HAL_Delay(200);
    HAL_HCD_ResetPort(&hhcd_);
    HAL_Delay(100);  // Wait for 100 ms after Reset

    // addDevice(NULL, 0, IsLowSpeedHCD());
  }

  void Request() {}
};
HcdImpl* hcd_impl;

}  // namespace
//* HAL Callbacks and IRQ Handler
extern "C" {
void HAL_HCD_Connect_Callback(HCD_HandleTypeDef* hhcd) {
  logger.Info("HAL_HCD_Connect_Callback");
  hcd_impl->Attached_();
}

void OTG_FS_IRQHandler(void) {
  logger.Info("OTG_FS_IRQHandler");
  hcd_impl->CallIRQHandler_();
}
}  // extern "C"

namespace stm32_usb {
HCD::HCD() {
  if (!hcd_impl) {
    hcd_impl = new HcdImpl;
  }
}

void HCD::Init() { hcd_impl->Init(); }
void HCD::WaitForAttach() { ::WaitForAttach(); }

HCD_HandleTypeDef* HCD::GetHandle() { return &hhcd_; }

HCD* HCD::instance = nullptr;
}  // namespace stm32_usb