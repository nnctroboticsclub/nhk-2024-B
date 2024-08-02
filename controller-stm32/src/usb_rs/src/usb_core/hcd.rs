use crate::binding::stm32_usb_HCD;

pub struct Hcd {
    hcd: stm32_usb_HCD,
}

impl Hcd {
    pub fn new() -> Hcd {
        Hcd {
            hcd: unsafe { stm32_usb_HCD::new() },
        }
    }

    pub fn init(&mut self) {
        unsafe {
            self.hcd.Init();
        }
    }

    pub fn wait_device(&mut self) {
        unsafe {
            self.hcd.WaitForAttach();
        }
    }

    pub fn reset_port(&mut self) {
        unsafe {
            self.hcd.ResetPort();
        }
    }
}

impl Drop for Hcd {
    fn drop(&mut self) {
        unsafe {
            self.hcd.destruct();
        }
    }
}
