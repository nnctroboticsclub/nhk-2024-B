use core::fmt::Debug;

use alloc::{fmt, format, string::String, vec::Vec};

use crate::{
    binding::{stm32_usb_host_HC, stm32_usb_host_HCStatus},
    logger::Logger,
    sleep_ms,
    usb::{HCError, RawURBStatus, URBError, URBResult},
};

use super::{HCStatus, RawHCStatus};

pub enum TransactionToken {
    Setup,
    In,
    Out,
}

impl Debug for TransactionToken {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            TransactionToken::Setup => write!(f, "Setup"),
            TransactionToken::In => write!(f, "   In"),
            TransactionToken::Out => write!(f, "  Out"),
        }
    }
}

impl TransactionToken {
    pub fn is_outgoing_token(&self) -> bool {
        match self {
            TransactionToken::Out => true,
            TransactionToken::Setup => true,

            _ => false,
        }
    }
}

pub struct TransactionDestination {
    pub dev: u8,
    pub ep: u8,
}

impl Debug for TransactionDestination {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{dev:02X}-{ep:02X}", dev = self.dev, ep = self.ep)
    }
}

pub struct Transaction<'a> {
    pub token: TransactionToken,
    pub dest: TransactionDestination,
    pub toggle: u8,
    pub buffer: &'a mut [u8],
    pub length: u8,
}

impl<'a> Debug for Transaction<'a> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(
            f,
            "{:?}[{:?}]<{}>{{{:?};",
            self.token, self.dest, self.toggle, self.length,
        )?;

        for i in 0..self.length {
            write!(f, " {:02X}", self.buffer[i as usize])?;
        }

        write!(f, "}}")
    }
}

pub struct HC {
    hc: stm32_usb_host_HC,
    dev: u8,
    ep: u8,

    logger: Logger,
}

impl HC {
    pub fn new(dev: u8, ep: u8) -> HC {
        HC {
            hc: unsafe { stm32_usb_host_HC::new() },
            dev,
            ep,
            logger: Logger::new(
                format!("ep{ep}.dev{dev}.host.usb"),
                format!("\x1b[32mUSB\x1b[34md{dev:02X}\x1b[35me{ep:02X}\x1b[m"),
            ),
        }
    }

    pub fn init(&mut self, speed: i32, ep_type: i32, max_packet_size: i32) {
        unsafe {
            self.hc.Init(
                self.ep as i32,
                self.dev as i32,
                speed,
                ep_type,
                max_packet_size,
            );
        }
    }

    pub fn submit_urb(&mut self, transaction: &mut Transaction) {
        let is_in = !transaction.token.is_outgoing_token();

        unsafe {
            let Transaction {
                ref token,
                dest: _,
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

            self.hc.DataToggle(*toggle as i32);
            self.hc
                .SubmitRequest(direction, pbuff, length, setup, do_ping)
        }
    }

    pub fn wait_done(&mut self) {
        let mut tick: u32 = 0;
        while self.get_urb_status() != RawURBStatus::Done {
            sleep_ms(10);

            if tick > 100 {
                self.logger.error("Timeout");
                break;
            }

            tick += 1;
        }
    }

    pub fn get_hc_status(&mut self) -> RawHCStatus {
        unsafe { self.hc.GetState() }.into()
    }

    pub fn get_urb_status(&mut self) -> RawURBStatus {
        unsafe { self.hc.GetURBState() }.into()
    }
}

impl Drop for HC {
    fn drop(&mut self) {
        unsafe {
            self.hc.destruct();
        }
    }
}
