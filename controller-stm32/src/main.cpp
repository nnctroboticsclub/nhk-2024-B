#include "mbed.h"
#include <robotics/logger/logger.hpp>

UnbufferedSerial pc(USBTX, USBRX, 115200);

robotics::logger::Logger logger{"otg.usb.nw", "   APP   "};

// 0: F401RE-USBHost
// 1: libusb_stm32
#define __MODE__ 1

#if __MODE__ == 0
#include "USBHostHID.h"

void App() {
  HC::EnableLog();
  auto hid = new USBHostHID;

  auto connect = hid->connect();
  logger.Info("connect --> %d", connect);
  logger.Info("connected = %d", hid->connected());
}
#elif __MODE__ == 1
#include "hcd.hpp"

/*
 * USB_OTG_FS GPIO Configuration
 * PA11: USB_OTG_FS_DM
 * PA12: USB_OTG_FS_DP
 */
//* HCD (USB Host Controller Driver)

void App() {
  stm32_usb::HCD* hcd = stm32_usb::HCD::GetInstance();
  hcd->Init();
  logger.Info("Init Done");
  hcd->WaitForAttach();

  logger.Info("USB Device Connected");
}
#endif

int main(void) {
  printf("\n\n\n\n\nProgram Started!!!!!\n");
  robotics::logger::core::StartLoggerThread();

  logger.Info("Start");
  App();
  while (1);
}
