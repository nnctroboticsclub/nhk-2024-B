#pragma once
#include "hcd.hpp"
#include "device.hpp"
#include "endpoint.hpp"

class App {
  static robotics::logger::Logger logger;
  stm32_usb::HCD *hcd;
  stm32_usb::host::Device device;
  stm32_usb::host::EndpointControl ep0;

  std::chrono::duration<int, std::milli> not_ready_delay = 10ms;

 public:
  App() : hcd(stm32_usb::HCD::GetInstance()), device(), ep0(device, 0) {}
  void Init() {
    hcd->Init();
    logger.Info("Init Done");

    hcd->WaitForAttach();
    logger.Info("USB Device Connected");

    device.SetAddress(0);
    ep0.SetMaxPacketSize(0x08);
  }

  uint8_t GetConfiguration() {
    //* Request
    uint8_t buffer[8] = {
        0b1'00'00000,        // bmRequestType: D->H, Standard, Device
        0x08,                // bRequest: GET_CONFIGURATION
        0x00,         0x00,  // wValue: 00

        0x00,         0x00,  // wIndex
        0x01,         0x00,  // wLength
    };

    // printf("\x1b[2J\x1b[H");
    logger.Info("Get Configuration");
    logger.Info("\x1b[42m                                             \x1b[m");
    while (true) {
      if (ep0.ControlRead(buffer, 8, 1) == HCD_URBStateTypeDef::URB_NOTREADY) {
        ThisThread::sleep_for(not_ready_delay);
        continue;
      }

      break;
    }

    logger.Info("Configuration: %d", buffer[0]);

    return buffer[0];
  }

  void GetDescriptor(uint8_t *buf, uint16_t type = 0x01, int size = 8) {
    //* Request
    uint8_t req[8] = {
        0b1'00'00000,        // bmRequestType: D->H, Standard, Device
        0x06,                // bRequest: GET_DESCRIPTOR
        0x00,         type,  // wValue: Device Descriptor, index: 0

        0x00,         0x00,  // wIndex
        size,         0x00,  // wLength
    };

    const char *type_string = type == 0x01   ? "Device"
                              : type == 0x02 ? "Configuration"
                              : type == 0x03 ? "String"
                              : type == 0x04 ? "Interface"
                              : type == 0x05 ? "Endpoint"
                                             : "Unknown";
    // printf("\x1b[2J\x1b[H");
    logger.Info("%s Descriptor (%d bytes)", type_string, size);
    logger.Info("\x1b[42m                                             \x1b[m");
    while (true) {
      memset(buf, 0, size);
      memcpy(buf, req, 8);

      if (ep0.ControlRead(buf, 8, size) == HCD_URBStateTypeDef::URB_NOTREADY) {
        ThisThread::sleep_for(not_ready_delay);
        continue;
      }

      break;
    }
    logger.Info("%s Descriptor", type_string);
    logger.Hex(robotics::logger::core::Level::kInfo, buf, size);
  }

  void SetAddress(uint8_t *buf, int address) {
    uint8_t req_set_addr[8] = {
        0b0'00'00000,        // bmRequestType: H->D, Standard, Device
        0x05,                // bRequest: SET_ADDRESS
        0x05,         0x00,  // wValue: 05

        0x00,         0x00,  // wIndex
        0x00,         0x00,  // wLength
    };

    // printf("\x1b[2J\x1b[H");
    logger.Info("Set Address");
    logger.Info("\x1b[42m                                             \x1b[m");
    while (true) {
      memset(buf, 0, 18);
      memcpy(buf, req_set_addr, 8);

      if (ep0.ControlWrite(buf, 8, nullptr, 0) ==
          HCD_URBStateTypeDef::URB_NOTREADY) {
        ThisThread::sleep_for(not_ready_delay);
        continue;
      }

      break;
    }
    logger.Hex(robotics::logger::core::Level::kInfo, buf, 8);
    device.SetAddress(address);
  }

  void Test() {
    uint8_t buf[1024] = {0};
    Init();
    printf("\n");

    HAL_HCD_ResetPort(hcd->GetHandle());
    ThisThread::sleep_for(100ms);

    memset(buf, 0, 1024);
    GetDescriptor(buf, 1, 8);
    ep0.SetMaxPacketSize(buf[7]);
    printf("\n");

    HAL_HCD_ResetPort(hcd->GetHandle());
    ThisThread::sleep_for(100ms);

    memset(buf, 0, 1024);
    SetAddress(buf, 0x01);
    printf("\n");
    ThisThread::sleep_for(10000ms);

    GetConfiguration();
    printf("\n");
    GetConfiguration();
    printf("\n");

    memset(buf, 0, 1024);
    GetDescriptor(buf, 1, 0x12);
    printf("\n");

    memset(buf, 0, 1024);
    GetDescriptor(buf, 2, 0x09);
    printf("\n");

    /* memset(buf, 0, 1024);
    GetDescriptor(buf, 2, 0x27);
    printf("\n"); */

    logger.Info("Test Done");
    logger.Hex(robotics::logger::core::Level::kInfo, buf, 128);
  }
};

robotics::logger::Logger App::logger{"otg.usb.nw", " APP  OTG"};