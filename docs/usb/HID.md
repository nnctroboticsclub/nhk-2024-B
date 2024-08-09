HID

# Interface 0 HID Descriptor
0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
0x09, 0x06, // Usage (Keyboard)
0xA1, 0x01, // Collection (Application)
0x05, 0x07, //   Usage Page (Kbrd/Keypad)
0x19, 0xE0, //   Usage Minimum (0xE0)
0x29, 0xE7, //   Usage Maximum (0xE7)
0x15, 0x00, //   Logical Minimum (0)
0x25, 0x01, //   Logical Maximum (1)
0x81, 0x02, //   Input [1x8] data
0x81, 0x01, //   Input [8x1] const
0x05, 0x08, //   Usage Page (LEDs)
0x19, 0x01, //   Usage Minimum (Num Lock)
0x29, 0x05, //   Usage Maximum (Kana)
0x91, 0x02, //   Output [5x1] data
0x91, 0x01, //   Output [1x3] const
0x15, 0x00, //   Logical Minimum (0)
0x25, 0x99, //   Logical Maximum (-103)
0x05, 0x07, //   Usage Page (Kbrd/Keypad)
0x19, 0x00, //   Usage Minimum (0x00)
0x29, 0x99, //   Usage Maximum (0x99)
0x81, 0x00, //   Input [6x8] data
0x05, 0x0C, //   Usage Page (Consumer)
0x09, 0x00, //   Usage (Unassigned)
0x15, 0x80, //   Logical Minimum (-128)
0x25, 0x7F, //   Logical Maximum (127)
0xB1, 0x02, //   Feature [8x8] data
0xC0,       // End Collection

# Interface 1 HID Descriptor
0x05, 0x01,       // Usage Page (Generic Desktop Ctrls)
0x09, 0x02,       // Usage (Mouse)
0xA1, 0x01,       // Collection (Application)
0x85, 0x01,       //   Report ID (1)
0x09, 0x01,       //   Usage (Pointer)
0xA1, 0x00,       //   Collection (Physical)
0x05, 0x09,       //     Usage Page (Button)
0x19, 0x01,       //     Usage Minimum (0x01)
0x29, 0x05,       //     Usage Maximum (0x05)
0x15, 0x00,       //     Logical Minimum (0)
0x25, 0x01,       //     Logical Maximum (1)
0x81, 0x02,       //     Input [1x5]
0x81, 0x01,       //     Input [3x1]
0x05, 0x01,       //     Usage Page (Generic Desktop Ctrls)
0x09, 0x30,       //     Usage (X)
0x09, 0x31,       //     Usage (Y)
0x09, 0x38,       //     Usage (Wheel)
0x15, 0x81,       //     Logical Minimum (-127)
0x25, 0x7F,       //     Logical Maximum (127)
0x81, 0x06,       //     Input [3x8]
0xC0,             //   End Collection
0xC0,             // End Collection
0x05, 0x01,       // Usage Page (Generic Desktop Ctrls)
0x09, 0x80,       // Usage (Sys Control)
0xA1, 0x01,       // Collection (Application)
0x85, 0x02,       //   Report ID (2)
0x05, 0x01,       //   Usage Page (Generic Desktop Ctrls)
0x19, 0x81,       //   Usage Minimum (Sys Power Down)
0x29, 0x83,       //   Usage Maximum (Sys Wake Up)
0x15, 0x00,       //   Logical Minimum (0)
0x25, 0x03,       //   Logical Maximum (3)
0x81, 0x40,       //   Input[8x1]
0xC0,             // End Collection
0x05, 0x0C,       // Usage Page (Consumer)
0x09, 0x01,       // Usage (Consumer Control)
0xA1, 0x01,       // Collection (Application)
0x85, 0x03,       //   Report ID (3)
0x19, 0x00,       //   Usage Minimum (Unassigned)
0x2A, 0x9C, 0x02, //   Usage Maximum (AC Distribute Vertically)
0x15, 0x00,       //   Logical Minimum (0)
0x26, 0x9C, 0x02, //   Logical Maximum (668)
0x81, 0x00,       //   Input [16x1]
0xC0,             // End Collection

// 104 bytes
