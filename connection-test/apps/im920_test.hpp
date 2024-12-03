#pragma once

#include <memory>

#include <mbed.h>

#include <logger/logger.hpp>
#include <robotics/thread/thread.hpp>
#include <srobo2/com/mbed_cstream.hpp>
#include <srobo2/com/im920.hpp>
#include <srobo2/timer/mbed_timer.hpp>

class IM920Test {
  [[noreturn]]
  void Task() const {
    auto logger = robotics::logger::Logger("main", "Main");

    auto uart = std::make_shared<mbed::UnbufferedSerial>(PA_9, PA_10, 19200);
    auto tx = std::make_shared<srobo2::com::UARTCStreamTx>(uart);
    auto rx = std::make_shared<srobo2::com::UARTCStreamRx>(uart);
    auto timer = std::make_shared<srobo2::timer::MBedTimer>();

    auto im920 = std::make_shared<srobo2::com::CIM920>(tx->GetTx(), rx->GetRx(),
                                                       timer->GetTime());

    auto node_number = im920->GetNodeNumber(1.0);
    logger.Info("Node Number: %d", node_number);

    uint16_t remote = 3 - node_number;

    auto version = im920->GetVersion(1.0);
    logger.Info("Version: %s", version.c_str());

    im920->OnData([&logger](uint16_t from, uint8_t* data, size_t len) {
      logger.Info("OnData: from %d", from);
      logger.Hex(robotics::logger::core::Level::kInfo, data, len);
    });

    while (true) {
      im920->Send(remote, (uint8_t*)"Hello", 5, 1.0);
      robotics::system::SleepFor(1s);
    }
  }

 public:
  [[noreturn]]
  void Main() const {
    auto thread = std::make_unique<robotics::system::Thread>();
    thread->SetStackSize(8192);
    thread->SetThreadName("App");
    thread->Start([this]() { this->Task(); });

    while (true) {
      robotics::system::SleepFor(100s);
    }
  }
};