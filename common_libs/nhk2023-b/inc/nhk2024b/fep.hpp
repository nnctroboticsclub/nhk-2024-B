#pragma once

#include <robotics/network/uart_stream.hpp>

#include <robotics/driver/dout.hpp>
#include <robotics/network/fep/fep_driver.hpp>

namespace nhk2024b {
robotics::logger::Logger fep_logger("fep.nhk2024b", "FEP   LOG");

void InitFEP(bool reset_fep = false, int self_address = 7) {
  robotics::network::UARTStream uart{PC_6, PC_7, 115200};
  robotics::driver::Dout rst{PC_9};
  robotics::driver::Dout ini{PC_8};
  robotics::network::fep::FEPDriver fep_drv{uart, rst, ini};

  if (reset_fep) {
    fep_logger.Info("Resetting FEP Registers (reason: btn3)");
    fep_drv.ResetRegistersHW();
  }

  // address
  fep_drv.AddConfiguredRegister(0, self_address);
  fep_drv.AddConfiguredRegister(1, 0xF0);

  // config
  fep_drv.AddConfiguredRegister(18, 0x8F);

  // ch
  fep_drv.AddConfiguredRegister(6, 1);     // only ch1
  fep_drv.AddConfiguredRegister(7, 0x1B);  // ch(1)
  fep_drv.AddConfiguredRegister(8, 0x27);  // ch(2)
  fep_drv.AddConfiguredRegister(9, 0x33);  // ch(3)

  // scramble
  fep_drv.AddConfiguredRegister(4, 0x20);
  fep_drv.AddConfiguredRegister(5, 0x48);
  fep_drv.ConfigureBaudrate(robotics::network::fep::FEPBaudrate(
      robotics::network::fep::FEPBaudrateValue::k115200));

  {
    auto result = fep_drv.Init();
    if (!result.IsOk()) {
      fep_logger.Error("Failed to init FEP Driver: %s",
                       result.UnwrapError().c_str());
    }
  }
}
}  // namespace nhk2024b