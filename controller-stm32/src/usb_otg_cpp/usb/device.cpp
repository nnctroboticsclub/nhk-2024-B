#include "device.hpp"

#include <mbed.h>
#include <stm32f4xx_hal_def.h>
#include <stm32f4xx_hal_hcd.h>

namespace stm32_usb::host {
int Device::GetSpeed() { return HCD_SPEED_LOW; }
}  // namespace stm32_usb::host