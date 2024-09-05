#include <mbed.h>
#include <PS4USB.h>


int main(int argc, char const *argv[])
{
  USB usb (D11, D12, D13, D10, D9);
PS4USB ps4 (&usb);


  printf("Initializing USB...\n");
  usb.Init();

  while(1) {
    printf("Task @%p\n", &usb);
     usb.Task();
    // if (ps4.connected())
    // printf("%d\n", ps4.getAnalogButton(ButtonEnum::L2));
  }
  return 0;
}
