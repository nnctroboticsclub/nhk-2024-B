#include "controller.hpp"

int main_switch() {
  printf("main() started\n");

  printf("Build information:\n");
  printf("  - Build date: %s\n", __DATE__);
  printf("  - Build time: %s\n", __TIME__);

  nhk2024b::ControllerReader controller_reader;

  return 0;
}
