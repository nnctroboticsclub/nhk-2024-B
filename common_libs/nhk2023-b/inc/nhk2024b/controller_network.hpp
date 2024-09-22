#pragma once

#include <robotics/network/ssp/ssp.hpp>
#include <robotics/network/ssp/value_store.hpp>
#include <robotics/network/ssp/keep_alive.hpp>

#include <nhk2024b/robot1/svc.hpp>
#include <nhk2024b/robot2/svc.hpp>
#include <nhk2024b/node_id.hpp>

#include <srobo2/com/im920_srobo1.hpp>
#include <srobo2/com/im920.hpp>

#ifdef MBED
#include <srobo2/com/mbed_cstream.hpp>
#include <srobo2/timer/mbed_timer.hpp>
#endif


namespace nhk2024b {
class ControllerNetwork {
 public:
  srobo2::com::IM920_SRobo1 *im920_ = nullptr;

  robotics::network::ssp::SerialServiceProtocol<uint16_t, bool> *ssp = nullptr;

  robotics::network::ssp::KeepAliveService<uint16_t, bool> *keep_alive =
      nullptr;
  robotics::network::ssp::ValueStoreService<uint16_t, bool> *value_store =
      nullptr;

 public:
  ControllerNetwork() {}

#ifdef MBED
  void IM920Init_Mbed() {
    auto tx = new srobo2::com::UARTCStreamTx(uart);
    auto rx = new srobo2::com::UARTCStreamRx(uart);
    auto timer = new srobo2::timer::MBedTimer();

    auto cim920 =
        new srobo2::com::CIM920(tx->GetTx(), rx->GetRx(), timer->GetTime());

    im920_ = new srobo2::com::IM920_SRobo1(cim920);
  }
#endif

  void IM920_UseAs(srobo2::com::IM920_SRobo1 &im920) {
    im920_ = &im920;
  }

  void Init(uint16_t node_number) {
    im920_->EnableWrite();
    im920_->SetNodeNumber(node_number);
    im920_->SetChannel(0x02);

    ssp = new robotics::network::ssp::SerialServiceProtocol<uint16_t, bool>(
        *im920_);

    value_store = ssp->RegisterService<
        robotics::network::ssp::ValueStoreService<uint16_t, bool>>();

    keep_alive = ssp->RegisterService<
        robotics::network::ssp::KeepAliveService<uint16_t, bool>>();

    value_store->OnNodeReceived([this](uint16_t _from, uint32_t _node_id) {
      keep_alive->TreatKeepAlive();
    });

    logger.Info("Node number: %04x", im920_->GetNodeNumber());
    logger.Info("Group number: %08x", im920_->GetGroupNumber());
  }

  nhk2024b::robot2::ControllerService<uint16_t, bool> *GetRobot2Service() {
    return ssp->RegisterService<nhk2024b::robot2::ControllerService<uint16_t, bool>>();
  }

  nhk2024b::robot1::ControllerService<uint16_t, bool> *GetRobot1Service() {
    return ssp->RegisterService<nhk2024b::robot1::ControllerService<uint16_t, bool>>();
  }
};
}  // namespace nhk2024b