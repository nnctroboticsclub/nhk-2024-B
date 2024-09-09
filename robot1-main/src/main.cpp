#include "main.hpp"

int main() {
  static bool guard = false;

  if (guard) {
    printf("!!!!! main() called twice !!!!!\n");
    printf("This may causes by stack smashing or double initialization.\n");
    // *((int*)0) = 0;  // cause hard fault
  }

  guard = true;

  printf("main() started\n");
  printf("Build: " __DATE__ " - " __TIME__ "\n");

  robotics::logger::Init();

  main_switch();

  printf("main() finished. Halting..");

  while (1) {
    ThisThread::sleep_for(1s);
  }

  return 0;
}
