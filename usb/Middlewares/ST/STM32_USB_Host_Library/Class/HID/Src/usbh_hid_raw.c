#include "usbh_hid_raw.h"
#include "usbh_hid_parser.h"

static USBH_StatusTypeDef USBH_HID_RawDecode(USBH_HandleTypeDef *phost);

void Test_syoch_01(USBH_HandleTypeDef *phost);

static uint8_t data_buffer[0x40];
static uint8_t fifo_buffer[0x40];

USBH_StatusTypeDef USBH_HID_RawInit(USBH_HandleTypeDef *phost) {
  uint32_t i;
  HID_HandleTypeDef *HID_Handle =
      (HID_HandleTypeDef *)phost->pActiveClass->pData;

  for (i = 0U; i < sizeof(data_buffer); i++) {
    data_buffer[i] = 0U;
    fifo_buffer[i] = 0U;
  }

  if (HID_Handle->length > sizeof(data_buffer)) {
    HID_Handle->length = (uint16_t)sizeof(data_buffer);
  }
  HID_Handle->pData = fifo_buffer;

  if ((HID_QUEUE_SIZE * sizeof(data_buffer)) > sizeof(phost->device.Data)) {
    return USBH_FAIL;
  } else {
    USBH_HID_FifoInit(&HID_Handle->fifo, phost->device.Data,
                      (uint16_t)(HID_QUEUE_SIZE * sizeof(data_buffer)));
  }

  return USBH_OK;
}

static USBH_StatusTypeDef USBH_HID_RawDecode(USBH_HandleTypeDef *phost) {
  HID_HandleTypeDef *HID_Handle =
      (HID_HandleTypeDef *)phost->pActiveClass->pData;

  if ((HID_Handle->length == 0U) || (HID_Handle->fifo.buf == NULL)) {
    return USBH_FAIL;
  }
  if (USBH_HID_FifoRead(&HID_Handle->fifo, &data_buffer, HID_Handle->length) !=
      HID_Handle->length) {
    return USBH_FAIL;
  }

  for (size_t i = 0; i < 0x40; i++) {
    printf("%02x ", data_buffer[i]);
  }
  printf("\n");

  return USBH_OK;
}

void Test_syoch_01(USBH_HandleTypeDef *phost) { USBH_HID_RawDecode(phost); }