use crate::{binding::stm32_usb_host_HC, logger::Logger};
use alloc::format;

use super::{
    urb_status::URBStatus, Transaction, TransactionDestination, TransactionResult,
    TransactionToken, HC,
};

pub struct BindedHC {
    hc: stm32_usb_host_HC,
    dest: TransactionDestination,
    ep_type: super::EPType,
    mps: u32,

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

impl BindedHC {
    fn init(&mut self) {
        unsafe {
            self.hc.Init(
                self.dest.ep as i32,
                self.dest.dev as i32,
                self.ep_type.into(),
                self.mps as i32,
            );
        }
    }
}

impl HC for BindedHC {
    fn new(dest: TransactionDestination, ep_type: super::EPType, max_packet_size: u32) -> Self {
        let logger = Logger::new(
            format!("ep{}.dev{}.host.usb", dest.ep, dest.dev),
            format!(
                "\x1b[32mUSB\x1b[34md{:02X}\x1b[35me{:02X}\x1b[m",
                dest.dev, dest.ep
            ),
        );

        let sys_hc = unsafe { stm32_usb_host_HC::new() };
        let mut hc = BindedHC {
            hc: sys_hc,
            dest,
            logger,
            ep_type,
            mps: max_packet_size,
        };

        hc.init();

        hc
    }

    fn set_max_packet_size(&mut self, max_packet_size: u32) {
        self.mps = max_packet_size;
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

        self.init();
        unsafe {
            self.hc.DataToggle(*toggle as i32);
            self.hc
                .SubmitRequest(direction, pbuff, length, setup, do_ping)
        }

        self.wait_done()?;

        Ok(())
    }

    fn get_urb_status(&mut self) -> URBStatus {
        unsafe { self.hc.GetURBState() }.into()
    }

    fn get_dest(&self) -> &TransactionDestination {
        &self.dest
    }

    fn get_dest_mut(&mut self) -> &mut TransactionDestination {
        &mut self.dest
    }
}

impl Drop for BindedHC {
    fn drop(&mut self) {
        unsafe {
            self.hc.destruct();
        }
    }
}
