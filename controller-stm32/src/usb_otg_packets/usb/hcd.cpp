#include "hcd.hpp"

#include <robotics/logger/logger.hpp>
#include <robotics/platform/thread.hpp>
#include <mbed.h>
#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_hcd.h>

extern "C" void OTG_FS_IRQHandler(void);
extern "C" void OTG_HS_IRQHandler(void);

extern "C" void HAL_HCD_MspInit(HCD_HandleTypeDef* hhcd) {
  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_SYSCFG_CLK_ENABLE();

  if (hhcd->Instance == USB_OTG_FS) {
    //* Enable clock
    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

    //* GPIO
    GPIO_InitTypeDef gpio;
    gpio.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_LOW;
    gpio.Alternate = GPIO_AF10_OTG_FS;
    HAL_GPIO_Init(GPIOA, &gpio);

    HAL_NVIC_SetPriority(OTG_FS_IRQn, 6, 0);
  } else if (hhcd->Instance == USB_OTG_HS) {
    //* Enable clock
    __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
    __HAL_RCC_USB_OTG_HS_ULPI_CLK_ENABLE();

    //* GPIO
    GPIO_InitTypeDef gpio;
    gpio.Pin = GPIO_PIN_14 | GPIO_PIN_15;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_LOW;
    gpio.Alternate = GPIO_AF12_OTG_HS_FS;
    HAL_GPIO_Init(GPIOB, &gpio);
  }
}

namespace {
//* Any USB device is connected to the USB port
volatile bool attach_done = false;

//* HCD Driver
HCD_HandleTypeDef hhcd_;

//* Logger
robotics::logger::Logger logger("hcd.host.usb",
                                "\x1b[32mUSB \x1b[33mHCD  \x1b[0m");

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

DigitalOut vbus(PC_0);

class HcdImpl {
  // robotics::system::Thread thread_;

 public:
  HcdImpl() {
    /* thread_.SetThreadName("HCD");
    thread_.SetStackSize(1024);
    thread_.Start([this]() {
      logger.Trace("HCD Thread Started");
      int frame = -1;
      while (1) {
        if (frame != HAL_HCD_GetCurrentFrame(&hhcd_)) {
          frame = HAL_HCD_GetCurrentFrame(&hhcd_);
          logger.Debug("HCD: Frame %d", frame);
        }
      }
    }); */
  }

  void CallIRQHandler_() { HAL_HCD_IRQHandler(&hhcd_); }

  void Attached_() { attach_done = true; }

  void Init() {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    vbus = 0;
    ThisThread::sleep_for(100ms);

    memset((void*)&hhcd_, 0, sizeof(hhcd_));
    hhcd_.Instance = USB_OTG_FS;
    hhcd_.Init.Host_channels = 8;
    hhcd_.Init.speed = HCD_SPEED_FULL;
    hhcd_.Init.dma_enable = DISABLE;
    hhcd_.Init.phy_itface = HCD_PHY_EMBEDDED;
    hhcd_.Init.Sof_enable = DISABLE;
    hhcd_.Init.low_power_enable = ENABLE;
    hhcd_.Init.vbus_sensing_enable = DISABLE;
    hhcd_.Init.use_external_vbus = DISABLE;

    NVIC_DisableIRQ(OTG_FS_IRQn);
    NVIC_SetVector(OTG_FS_IRQn, (uint32_t)&OTG_FS_IRQHandler);

    // NVIC_DisableIRQ(OTG_HS_IRQn);
    // NVIC_SetVector(OTG_HS_IRQn, (uint32_t)&OTG_HS_IRQHandler);
    auto ret = HAL_HCD_Init(&hhcd_);
    if (ret != HAL_StatusTypeDef::HAL_OK) {
      logger.Error("HAL_HCD_Init failed with %d", ret);
      return;
    }
    NVIC_EnableIRQ(OTG_FS_IRQn);
    // NVIC_EnableIRQ(OTG_HS_IRQn);

    hhcd_.pData = this;

    ret = HAL_HCD_Start(&hhcd_);
    if (ret != HAL_StatusTypeDef::HAL_OK) {
      logger.Error("HAL_HCD_Init failed with %d", ret);
      return;
    }

    ThisThread::sleep_for(100ms);

    vbus = 1;
    ThisThread::sleep_for(100ms);
  }
  void WaitAttach() {
    using namespace std::chrono_literals;

    ::WaitForAttach();
    logger.Info("HAL_HCD_GetCurrentSpeed --> %d",
                HAL_HCD_GetCurrentSpeed(&hhcd_));
  }

  void ResetPort() { HAL_HCD_ResetPort(&hhcd_); }
};
HcdImpl* hcd_impl;

}  // namespace
//* HAL Callbacks and IRQ Handler
extern "C" {
void HAL_HCD_Connect_Callback(HCD_HandleTypeDef* hhcd) {
  hcd_impl->Attached_();
}

void OTG_FS_IRQHandler(void) { hcd_impl->CallIRQHandler_(); }
void OTG_HS_IRQHandler(void) { hcd_impl->CallIRQHandler_(); }
}  // extern "C"

namespace stm32_usb {
HCD::HCD() {
  if (!hcd_impl) {
    hcd_impl = new HcdImpl;
  }
}
HCD::~HCD() { delete hcd_impl; }

void HCD::Init() { hcd_impl->Init(); }
void HCD::WaitForAttach() { hcd_impl->WaitAttach(); }

void HCD::ResetPort() { hcd_impl->ResetPort(); }

void* HCD::GetHandle() { return &hhcd_; }

HCD* HCD::instance = nullptr;
}  // namespace stm32_usb