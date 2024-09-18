#pragma once

#include <robotics/utils/neopixel.hpp>
#include <robotics/utils/neopixel_driver.hpp>
#include <robotics/platform/spi.hpp>

namespace nhk2024b::led_tape {

std::shared_ptr<robotics::utils::NeoPixel> led;
void Init() {
  auto spi =
      std::make_shared<robotics::datalink::SPI>(PC_3, NC, NC, NC, (int)6.4e6);
  auto led_drv = std::make_shared<robotics::utils::NeoPixelSPIDriver>(spi);
  led = std::make_shared<robotics::utils::NeoPixel>(led_drv, 60);
}

void ColorOrange() {
  led->Clear();
  for (int i = 0; i < 60; i++) {
    led->PutPixel(i, 0xee7800);
  }
}

void ColorLightBlue() {
  led->Clear();
  for (int i = 0; i < 60; i++) {
    led->PutPixel(i, 0x33ccff);
  }
}

void Write() { led->Write(); }
}  // namespace nhk2024b::led_tape