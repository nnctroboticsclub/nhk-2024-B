#include "mbed.h"
#include <robotics/logger/logger.hpp>
#include "otg/app.hpp"

UnbufferedSerial pc(USBTX, USBRX, 115200);

robotics::logger::Logger logger{".", "\x1b[42m \x1b[0mAPP     "};

void RunApp() {
  App app;
  app.Test();
}

int main(void) {
  printf("\n\n\n\n\nProgram Started!!!!!\n");
  robotics::logger::core::StartLoggerThread();

  logger.Info("Start");
  logger.Info("Build: %s %s", __DATE__, __TIME__);
  RunApp();
  while (1);
}
