use crate::binding::stm32_usb_HCD;

use super::Hcd;

pub struct BindedHcd {
    hcd: stm32_usb_HCD,
}

impl Hcd for BindedHcd {
    fn new() -> BindedHcd {
        BindedHcd {
            hcd: unsafe { stm32_usb_HCD::new() },
        }
    }

    fn init(&mut self) {
        unsafe {
            self.hcd.Init();
        }
    }

    fn wait_device(&mut self) {
        unsafe {
            self.hcd.WaitForAttach();
        }
    }

    fn reset_port(&mut self) {
        unsafe {
            self.hcd.ResetPort();
        }
    }
}

impl Drop for BindedHcd {
    fn drop(&mut self) {
        unsafe {
            self.hcd.destruct();
        }
    }
}
