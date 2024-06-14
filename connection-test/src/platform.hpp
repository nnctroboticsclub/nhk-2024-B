#pragma once

#include <cstdint>

//* ####################
//* Platform
//* ####################

namespace platform {
enum class Mode {
  kDevice1,
  kDevice2,
};

Mode GetMode();

struct FEPConfig {
  uint8_t address;
  uint8_t group_address;
};
FEPConfig GetFEPConfiguration(Mode mode) {
  return mode == Mode::kDevice1 ? FEPConfig{1, 0xF0} : FEPConfig{2, 0xF0};
}

uint8_t GetRemoteTestAddress(Mode mode) {
  return mode == Mode::kDevice1 ? 2 : 1;
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

platform::Mode platform::GetMode() { return platform::Mode::kDevice1; }

#else

#include <mbed-robotics/uart_stream.hpp>
#include <mbed.h>
platform::Mode platform::GetMode() {
  mbed::DigitalIn mode(PA_9);

  return mode.read() ? platform::Mode::kDevice1 : platform::Mode::kDevice2;
}

#endif