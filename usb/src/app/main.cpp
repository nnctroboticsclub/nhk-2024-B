#include <mbed.h>

// This instance is must be on the global scope(.rodata)
/* USB usb(D11, D12, D13, D10, D9);
PS4USB ps4(&usb);
HIDUniversal hid(&usb); */

enum Register {
  PERIPHERAL_EP0FIFO = 0,

  PERIPHERAL_EP1OUTFIFO = 1,
  HOST_RCVFIFO = 1,

  PERIPHERAL_EP2INFIFO = 2,
  HOST_SNDFIFO = 2,

  PERIPHERAL_EP3INFIFO = 3,

  SUDFIFO = 4,

  PERIPHERAL_EP0BC = 5,

  PERIPHERAL_EP1OUTBC = 6,
  HOST_RCVBC = 6,

  PERIPHERAL_EP2INBC = 7,
  HOST_SNDBC = 7,

  PERIPHERAL_EP3INBC = 8,

  PERIPHERAL_EPSTALLS = 9,

  PERIPHERAL_CLRTOGS = 10,

  PERIPHERAL_EPIRQ = 11,

  PERIPHERAL_EPIEN = 12,

  USBIRQ = 13,
  USBIEN = 14,
  USBCTL = 15,
  CPUCTL = 16,
  PINCTL = 17,
  REVISION = 18,

  PERIPHERAL_FNADDR = 19,

  IOPINS1 = 20,
  IOPINS2 = 21,
  GPINIRQ = 22,
  GPINIEN = 23,
  GPINPOL = 24,

  HOST_HIRQ = 25,

  HOST_HIEN = 26,
  MODE = 27,

  HOST_PERADDR = 28,

  HOST_HCTL = 29,

  HOST_HXFR = 30,

  HOST_HRSL = 31,
};

class Max3421e {
  mbed::SPI spi;
  mbed::DigitalOut nss_;
  mbed::DigitalOut int_;

 public:
  Max3421e() : spi(D11, D12, D13), nss_(D10), int_(D9) {
    nss_ = 1;
    spi.format(8, 0);
    spi.frequency(26000000);  // 12 Mhz
  }

  void regWr(Register reg, uint8_t data) {
    uint8_t t[2] = {(static_cast<uint8_t>(reg) << 3) | 0x02,  // RRRR R0WA
                    data};
    uint8_t r[2];
    nss_ = 0;
    spi.write(t, 2, r, 2);
    nss_ = 1;

    printf("[SPI] regWr; tr; %02x %02x <=> %02x %02x\n", t[0], t[1], r[0],
           r[1]);
  }

  uint8_t regRd(Register reg) {
    uint8_t t[2] = {(static_cast<uint8_t>(reg) << 3) | 0x00,  // RRRR R0WA
                    0};
    uint8_t r[2];
    nss_ = 0;
    spi.write(t, 2, r, 2);
    nss_ = 1;

    printf("[SPI] regRd; tr; %02x %02x <=> %02x %02x\n", t[0], t[1], r[0],
           r[1]);

    return r[1];
  }

  void Task() {
    if (int_ == 0) {
      printf("INT\n");
    }
  }
};

Max3421e max3421e;

mbed::BufferedSerial pc(USBTX, USBRX, 115200);

int main(int argc, char const *argv[]) {
  uint8_t regs[32] = {
      0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,  //
      0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,  //
      0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,  //
      0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,  //
  };

  printf("Hello!\n");

  max3421e.regRd(Register::REVISION);
  max3421e.regRd(Register::REVISION);
  max3421e.regRd(Register::REVISION);

  max3421e.regWr(Register::MODE, 0x01);
  max3421e.regWr(Register::MODE, 0x01);
  max3421e.regWr(Register::MODE, 0x01);

  printf("Revision: %02x\n", max3421e.regRd(Register::REVISION));
  printf("Mode: %02x\n", max3421e.regRd(Register::MODE));

  while (1) {
    // max3421e.Task();
  }

  return 0;
}
