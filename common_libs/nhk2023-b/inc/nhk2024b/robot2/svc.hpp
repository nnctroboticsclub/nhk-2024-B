#pragma once

#include <unordered_map>

#include <robotics/logger/logger.hpp>
#include <robotics/network/stream.hpp>
#include <robotics/network/ssp/ssp.hpp>
#include <robotics/node/node.hpp>

#include "controller.hpp"

namespace nhk2024b::robot2 {
template <typename Context, typename TxRet = void>
class ControllerService : public robotics::network::ssp::SSP_Service<Context, TxRet> {
    Controller controller;
 public:
  Robot2SVC(robotics::network::Stream<uint8_t, Context, TxRet>& stream)
      : SSP_Service<Context, TxRet>(stream, 0x04, "svc.robot2.nhk2024b",
                                    "\x1b[32mRobot2SVC\x1b[m") {
    this->OnReceive(
        [this](Context addr, uint8_t* data, size_t len) {
          controller.Unpack(data);
        });
  }

  void Send(Context remote){
    auto payload = controller.Pack();
    this->Send(remote, payload, 3);
  }


};
}  // namespace robotics::network::ssp