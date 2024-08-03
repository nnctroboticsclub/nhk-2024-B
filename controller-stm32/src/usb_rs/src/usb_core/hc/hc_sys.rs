use crate::{binding::stm32_usb_host_HC, logger::Logger};
use alloc::format;

use super::{
    urb_status::URBStatus, Transaction, TransactionDestination, TransactionResult,
    TransactionToken, HC,
};

pub struct BindedHC {
    hc: stm32_usb_host_HC,
    dest: TransactionDestination,

    logger: Logger,
}

impl Into<i32> for super::EPType {
    fn into(self) -> i32 {
        match self {
            super::EPType::Control => 0,
            super::EPType::Isochronous => 1,
            super::EPType::Bulk => 2,
            super::EPType::Interrupt => 3,
        }
    }
}

impl HC for BindedHC {
    fn new(dest: TransactionDestination, ep_type: super::EPType, max_packet_size: i32) -> Self {
        let logger = Logger::new(
            format!("ep{}.dev{}.host.usb", dest.ep, dest.dev),
            format!(
                "\x1b[32mUSB\x1b[34md{:02X}\x1b[35me{:02X}\x1b[m",
                dest.dev, dest.ep
            ),
        );

        let mut hc = unsafe { stm32_usb_host_HC::new() };
        unsafe {
            hc.Init(
                dest.ep as i32,
                dest.dev as i32,
                ep_type.into(),
                max_packet_size,
            );
        }

        BindedHC { hc, dest, logger }
    }

    fn submit_urb(&mut self, transaction: &mut Transaction) -> TransactionResult<()> {
        let is_in = !transaction.token.is_outgoing_token();

        let Transaction {
            ref token,
            toggle,
            ref mut buffer,
            length: length_,
        } = transaction;

        let direction = if is_in { 1 } else { 0 };
        let pbuff = buffer.as_mut_ptr();
        let length = *length_ as i32;
        let setup = match token {
            TransactionToken::Setup => true,
            _ => false,
        };
        let do_ping = false;

        unsafe {
            self.hc.DataToggle(*toggle as i32);
            self.hc
                .SubmitRequest(direction, pbuff, length, setup, do_ping)
        }

        self.wait_done()
    }

    fn get_urb_status(&mut self) -> URBStatus {
        unsafe { self.hc.GetURBState() }.into()
    }
}

impl Drop for BindedHC {
    fn drop(&mut self) {
        unsafe {
            self.hc.destruct();
        }
    }
}
