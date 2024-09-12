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

  void Init() {
    auto uart = std::make_shared<mbed::UnbufferedSerial>(PA_9, PA_10, 19200);

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