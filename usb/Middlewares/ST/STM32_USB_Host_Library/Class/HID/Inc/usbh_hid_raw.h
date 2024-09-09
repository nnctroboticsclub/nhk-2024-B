#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "usbh_hid.h"

USBH_StatusTypeDef USBH_HID_RawInit(USBH_HandleTypeDef *phost);
uint8_t *USBH_HID_RawGetReport(USBH_HandleTypeDef *phost);

#ifdef __cplusplus
}
#endif