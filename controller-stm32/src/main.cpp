#include "mbed.h"
#include <robotics/logger/logger.hpp>

#include "hcd.hpp"
#include "device.hpp"
#include "endpoint.hpp"

UnbufferedSerial pc(USBTX, USBRX, 115200);

robotics::logger::Logger logger{"otg.usb.nw", "   APP   "};

void App() {
  stm32_usb::HCD* hcd = stm32_usb::HCD::GetInstance();
  hcd->Init();
  logger.Info("Init Done");
  hcd->WaitForAttach();

  stm32_usb::host::Device device;
  device.SetAddress(0);

  stm32_usb::host::EndpointControl ep0(device, 0);
  uint8_t buf[8] = {
      //* Request
      0b1'00'00000,        // bmRequestType: D->H, Standard, Device
      0x06,                // bRequest: GET_DESCRIPTOR
      0x01,         0x00,  // wValue: Device Descriptor, index: 0

      0x00,         0x00,  // wIndex
      0x00,         0x40,  // wLength

  };
  ep0.ControlRead(buf, 8);

  logger.Hex(robotics::logger::core::Level::kInfo, buf, 8);

  logger.Info("USB Device Connected");
}

int main(void) {
  printf("\n\n\n\n\nProgram Started!!!!!\n");
  robotics::logger::core::StartLoggerThread();

  logger.Info("Start");
  logger.Info("Build: %s %s", __DATE__, __TIME__);
  App();
  while (1);
}
