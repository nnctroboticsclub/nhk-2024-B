#include <mbed.h>

#include <PS4USB.h>

// This instance is must be on the global scope(.rodata)
USB usb(D11, D12, D13, D10, D9);
PS4USB ps4(&usb);

int main(int argc, char const *argv[]) {
  printf("USB Host Shield PS4 Test\n");
  usb.USBInit();
  usb.Init();

  usb.busprobe();

  while (1) {
    usb.Task();

    if (ps4.connected()) {
      printf("PS4 connected\n");
    }
  }

  return 0;
}
