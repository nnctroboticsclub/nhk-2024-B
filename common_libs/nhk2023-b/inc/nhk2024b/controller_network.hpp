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

  {
    char tx_buf[] = "ENWR\r\n";
    uart.write(tx_buf, 6);
    printf("--> %02x %02x %02x %02x", tx_buf[0], tx_buf[1], tx_buf[2], tx_buf[3]);

    char rx_buf[4] = {0};
    uart.read(rx_buf, 4);
    printf("<-- %02x %02x %02x %02x")
  }

  {
    char tx_buf[] = "STNN0000\r\n";
    buf[4] = "0123456789ABCDEF"[(node_number >> 12) & 0x0f];
    buf[5] = "0123456789ABCDEF"[(node_number >> 8) & 0x0f];
    buf[6] = "0123456789ABCDEF"[(node_number >> 4) & 0x0f];
    buf[7] = "0123456789ABCDEF"[(node_number >> 0) & 0x0f];
    uart.write(tx_buf, 6);
    printf("--> %02x %02x %02x %02x", tx_buf[0], tx_buf[1], tx_buf[2], tx_buf[3]);

    char rx_buf[4] = {0};
    uart.read(rx_buf, 4);
    printf("<-- %02x %02x %02x %02x")
  }


    {
      static char buf[0x19];
      buf[0x00] = 'E';
      buf[0x01] = 'N';
      buf[0x02] = 'W';
      buf[0x03] = 'R';
      buf[0x04] = '\r';
      buf[0x05] = '\n';
      buf[0x06] = 'S';
      buf[0x07] = 'T';

      buf[0x08] = 'N';
      buf[0x09] = 'N';
      buf[0x0A] = "0123456789ABCDEF"[(node_number >> 12) & 0x0f];
      buf[0x0B] = "0123456789ABCDEF"[(node_number >> 8) & 0x0f];
      buf[0x0C] = "0123456789ABCDEF"[(node_number >> 4) & 0x0f];
      buf[0x0D] = "0123456789ABCDEF"[(node_number >> 0) & 0x0f];
      buf[0x0E] = '\r';
      buf[0x0F] = '\n';

      buf[0x10] = 'S';
      buf[0x11] = 'T';
      buf[0x12] = 'C';
      buf[0x13] = 'H';
      buf[0x14] = '0';
      buf[0x15] = '2';
      buf[0x16] = '\r';
      buf[0x17] = '\n';

      memset(print_buf, 0, sizeof(print_buf));
      for(int i = 0; i < 0x18; i++) {
        print_buf[2 * i + 0] = "0123456789ABCDEF"[(buf[i] >> 4) & 0x0f];
        print_buf[2 * i + 1] = "0123456789ABCDEF"[(buf[i] >> 0) & 0x0f];
      }
      robotics::system::SleepFor(500ms);
      printf("--> %s\n", print_buf);

      uart->write(buf, 0x19);
    }

    {
      static char buf[12];

      printf("Wait...\n");
      robotics::system::SleepFor(500ms);
      printf("Reading...\n");
      auto len = uart->read(buf, 12);

      memset(print_buf, 0, sizeof(print_buf));
      for(int i = 0; i < 12; i++) {
        print_buf[2 * i + 0] = "0123456789ABCDEF"[(buf[i] >> 4) & 0x0f];
        print_buf[2 * i + 1] = "0123456789ABCDEF"[(buf[i] >> 0) & 0x0f];
      }

      robotics::system::SleepFor(500ms);
      printf("<-- %s [%d]\n", print_buf, len);
    }

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