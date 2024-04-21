#include <cinttypes>
#include <mbed.h>

#include "identify.h"
#include "app.hpp"

#include <mbed-robotics/simple_can.hpp>

int main_0() { return 0; }

int main_1() { return 0; }

int main_2() { return 0; }

int main_3() { return 0; }

int main_pro() {
  App::Config config{        //
                     .com =  //
                     {
                         .can =
                             {
                                 .id = CAN_ID,
                                 .freqency = (int)1E6,
                                 .rx = PB_8,
                                 .tx = PB_9,
                             },
                         .driving_can =
                             {
                                 .rx = PB_5, /* PB_5, */
                                 .tx = PB_6, /* PB_6, */
                             },
                         .i2c =
                             {
                                 .sda = PC_9,
                                 .scl = PA_8,
                             },

                         .value_store_config = {},
                     },
                     .can1_debug = false};

  App app(config);
  app.Init();

  while (1) {
    ThisThread::sleep_for(100s);
  }

  return 0;
}

int main_switch() {
  printf("main() started\n");
  printf("Build: " __DATE__ " - " __TIME__ "\n");

  main_pro();
  return 0;
}
