#pragma once

#include <cstdint>

#ifndef __TEST_ON_HOST__
#include <robotics/network/uart_stream.hpp>
#include <mbed.h>
#endif

//* ####################
//* Platform
//* ####################

namespace platform {
enum class Mode {
  kDevice1,
  kDevice2,
};

struct {
  Mode mode;
  bool initialized = false;
} _mode;

Mode _GetMode();

Mode GetMode() {
  if (!_mode.initialized) {
    _mode.mode = _GetMode();
    _mode.initialized = true;
  }
  return _mode.mode;
}

struct FEPConfig {
  uint8_t address;
  uint8_t group_address;
};
FEPConfig GetFEPConfiguration() {
  return GetMode() == Mode::kDevice1 ? FEPConfig{1, 0xF0} : FEPConfig{2, 0xF0};
}

uint8_t GetRemoteTestAddress() { return GetMode() == Mode::kDevice1 ? 2 : 1; }

uint8_t GetSelfAddress() { return GetMode() == Mode::kDevice1 ? 1 : 2; }
const char* GetDeviceName() {
  return GetMode() == platform::Mode::kDevice1 ? "Device1" : "Device2";
}
}  // namespace platform

#if defined(__TEST_ON_HOST__)
namespace robotics::network {
class UARTStream : public Stream<uint8_t> {
  void Send(uint8_t* data, uint32_t len) override {
    printf("TX: ");
    for (size_t i = 0; i < len; i++) {
      printf("%02X ", data[i]);
    }
    printf("\n");
  }
};
}  // namespace robotics::network

platform::Mode platform::_GetMode() { return platform::Mode::kDevice1; }

#else

const mbed::BufferedSerial pc{USBTX, USBRX, 115200};

platform::Mode platform::_GetMode() {
  mbed::DigitalIn mode(PA_9, PinMode::PullDown);
  //
  return mode.read() ? platform::Mode::kDevice1 : platform::Mode::kDevice2;
}

#endif