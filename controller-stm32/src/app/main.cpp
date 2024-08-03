#include "mbed.h"
#include <robotics/logger/logger.hpp>
#include <robotics/platform/thread.hpp>
// #include "app.hpp"

UnbufferedSerial pc(USBTX, USBRX, 115200);

robotics::logger::Logger logger{".", "\x1b[42m \x1b[0mAPP     "};

extern "C" void usb_rs_run();

void RunApp() {
  logger.Info("Started App thread");
  // App app;
  // app.Test();
  usb_rs_run();
}

int main(void) {
  printf("\n\n\n\n\nProgram Started!!!!!\n");
  robotics::logger::core::StartLoggerThread();

  logger.Info("Start");
  logger.Info("Build: %s %s", __DATE__, __TIME__);
  // RunApp();
  // Thread thread{}
  robotics::system::Thread thread;
  thread.SetStackSize(16 * 1024);
  thread.SetThreadName("App");
  thread.Start(RunApp);
  while (1);
}
