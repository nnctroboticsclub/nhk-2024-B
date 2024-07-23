use crate::{
    binding::{stm32_usb_host_HC, stm32_usb_host_HCStatus},
    usb::RawURBStatus,
};

use super::RawHCStatus;

pub struct HC {
    hc: stm32_usb_host_HC,
    ep: u8,
}

impl HC {
    pub fn new() -> HC {
        HC {
            hc: unsafe { stm32_usb_host_HC::new() },
            ep: 0,
        }
    }

    pub fn init(&mut self, ep: i32, dev: i32, speed: i32, ep_type: i32, max_packet_size: i32) {
        unsafe {
            self.hc.Init(ep, dev, speed, ep_type, max_packet_size);
        }

        self.ep = ep as u8;
    }

    pub fn submit_urb(
        &mut self,
        is_in: bool,
        buffer: &mut [u8],
        length: i32,
        setup: bool,
        do_ping: bool,
    ) {
        let direction = if is_in { 1 } else { 0 };
        let pbuff = buffer.as_mut_ptr();
        unsafe {
            self.hc
                .SubmitRequest(direction, pbuff, length, setup, do_ping)
        }
    }

    pub fn data01(&mut self, toggle: bool) {
        unsafe {
            self.hc.Data011(toggle);
        }
    }

    fn get_hc_status(&mut self) -> RawHCStatus {
        unsafe { self.hc.GetState() }.into()
    }

    fn get_urb_status(&mut self) -> RawURBStatus {
        unsafe { self.hc.GetURBState() }.into()
    }

    pub fn status(&mut self) -> HCStatus {
        use RawHCStatus::*;
        use RawURBStatus::*;

        let hc_status = self.get_hc_status();
        let urb_status = self.get_urb_status();

        match (hc_status, urb_status) {
            (RawHCStatus::Idle, _) => URBResult::Success.into(),
            (RawHCStatus::Done, _) => URBResult::Success.into(),

            (UrbFailed, NotReady) => HCStatus::Done(URBError::Nak.into()),
            (UrbFailed, NYet) => HCStatus::Done(URBError::NYet.into()),
            (UrbFailed, Error) => HCStatus::Done(URBError::Halt.into()),
            (UrbFailed, Stall) => HCStatus::Done(URBError::Stall.into()),

            (XActErr, _) => URBResult::TransactError.into(),
            (BabbleErr, _) => HCStatus::Done(URBError::Babble.into()),
            (DataToggleErr, _) => HCStatus::Done(URBError::DataToggle.into()),

            (_, _) => HCStatus::Unknown,
        }
    }

    pub fn wait_urb_done(&mut self) {
        loop {
            let status = self.status();
            match status {
                HCStatus::Done(_) => break,
                _ => sleep_ms(10),
            }
        }
    }
}

impl Drop for HC {
    fn drop(&mut self) {
        unsafe {
            self.hc.destruct();
        }
    }
}
