#pragma once

#include <robotics/network/ssp/ssp.hpp>
#include <robotics/network/ssp/value_store.hpp>
#include <robotics/network/ssp/keep_alive.hpp>

#include <nhk2024b/robot1/controller.hpp>
#include <nhk2024b/robot2/controller.hpp>
#include <nhk2024b/node_id.hpp>

#include <srobo2/com/im920_srobo1.hpp>
#include <srobo2/com/im920.hpp>
#include <srobo2/com/mbed_cstream.hpp>
#include <srobo2/timer/mbed_timer.hpp>

namespace nhk2024b {
class ControllerNetwork {
  srobo2::com::IM920_SRobo1 *im920 = nullptr;

  robotics::network::ssp::SerialServiceProtocol<uint16_t, bool> *ssp = nullptr;

 public:
  robotics::network::ssp::KeepAliveService<uint16_t, bool> *keep_alive =
      nullptr;
  robotics::network::ssp::ValueStoreService<uint16_t, bool> *value_store =
      nullptr;

 public:
  ControllerNetwork() {}

  void Init(uint16_t node_number) {
    static char print_buf[512];

    auto uart = std::make_shared<mbed::UnbufferedSerial>(PA_9, PA_10, 19200);


    ThisThread::sleep_for(200ms);

    /* {
      char tx_buf[] = "ENWR\r\n";
      uart->write(tx_buf, 6);
      printf("--> %02x %02x %02x %02x %02x %02x\n", tx_buf[0], tx_buf[1], tx_buf[2], tx_buf[3], tx_buf[4], tx_buf[5]);

      ThisThread::sleep_for(50ms);

      char rx_buf[4] = {0};
      uart->read(&rx_buf[0], 1);
      uart->read(&rx_buf[1], 1);
      uart->read(&rx_buf[2], 1);
      uart->read(&rx_buf[3], 1);
      printf("<-- %02x %02x %02x %02x\n", rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3]);
    }

    {
      char tx_buf[] = "STNN0000\r\n";
      tx_buf[4] = "0123456789ABCDEF"[(node_number >> 12) & 0x0f];
      tx_buf[5] = "0123456789ABCDEF"[(node_number >> 8) & 0x0f];
      tx_buf[6] = "0123456789ABCDEF"[(node_number >> 4) & 0x0f];
      tx_buf[7] = "0123456789ABCDEF"[(node_number >> 0) & 0x0f];
      uart->write(tx_buf, 10);
      printf("--> %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
        tx_buf[0], tx_buf[1], tx_buf[2], tx_buf[3], tx_buf[4],
        tx_buf[5], tx_buf[6], tx_buf[7], tx_buf[8], tx_buf[9]
      );

      ThisThread::sleep_for(50ms);

      char rx_buf[4] = {0};
      uart->read(&rx_buf[0], 1);
      uart->read(&rx_buf[1], 1);
      uart->read(&rx_buf[2], 1);
      uart->read(&rx_buf[3], 1);
      printf("<-- %02x %02x %02x %02x\n", rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3]);
    }

    {
      char tx_buf[] = "STCH02\r\n";
      uart->write(tx_buf, 8);
      printf("--> %02x %02x %02x %02x %02x %02x %02x %02x\n",
        tx_buf[0], tx_buf[1], tx_buf[2], tx_buf[3],
        tx_buf[4], tx_buf[5], tx_buf[6], tx_buf[7]
      );

      ThisThread::sleep_for(50ms);

      char rx_buf[4] = {0};
      uart->read(&rx_buf[0], 1);
      uart->read(&rx_buf[1], 1);
      uart->read(&rx_buf[2], 1);
      uart->read(&rx_buf[3], 1);
      printf("<-- %02x %02x %02x %02x\n", rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3]);
    } */

    ThisThread::sleep_for(500ms);

    auto tx = new srobo2::com::UARTCStreamTx(uart);
    auto rx = new srobo2::com::UARTCStreamRx(uart);
    auto timer = new srobo2::timer::MBedTimer();

    auto cim920 =
        new srobo2::com::CIM920(tx->GetTx(), rx->GetRx(), timer->GetTime());

    im920 = new srobo2::com::IM920_SRobo1(cim920);

    ssp = new robotics::network::ssp::SerialServiceProtocol<uint16_t, bool>(
        *im920);

    value_store = ssp->RegisterService<
        robotics::network::ssp::ValueStoreService<uint16_t, bool>>();

    keep_alive = ssp->RegisterService<
        robotics::network::ssp::KeepAliveService<uint16_t, bool>>();

    value_store->OnNodeReceived([this](uint16_t _from, uint32_t _node_id) {
      keep_alive->TreatKeepAlive();
    });

    logger.Info("Node number: %04x", im920->GetNodeNumber());
    logger.Info("Group number: %08x", im920->GetGroupNumber());
    logger.Info("Channel: %02x", im920->GetChannel());
  }

  nhk2024b::robot2::Controller *ConnectToPipe2() {
    auto self = im920->GetNodeNumber();
    auto remote = nhk2024b::node_id::GetPipe2Remote(self);

    auto ctrl = new nhk2024b::robot2::Controller;

    ctrl->RegisterTo(value_store, remote);

    keep_alive->AddTarget(remote);
    return ctrl;
  }

  nhk2024b::robot1::Controller *ConnectToPipe1() {
    auto self = im920->GetNodeNumber();
    auto remote = nhk2024b::node_id::GetPipe1Remote(self);

    auto ctrl = new nhk2024b::robot1::Controller;

    ctrl->RegisterTo(value_store, remote);

    keep_alive->AddTarget(remote);
    return ctrl;
  }
};
}  // namespace nhk2024b