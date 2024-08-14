use core::marker::PhantomData;

use alloc::{boxed::Box, format};

use crate::{
    common::log,
    logger::{Logger, LoggerLevel},
};

use super::{
    hc::{EPType, Transaction, TransactionDestination, TransactionResult, TransactionToken, HC},
    std_request::StdRequest,
};

pub struct ControlEP<H: HC> {
    logger: Logger,
    pub(super) hc: Box<H>,

    dest: TransactionDestination,
    mps: u8,
    toggle: u8,
}

impl<H: HC> ControlEP<H> {
    pub fn new(hc: Box<H>) -> ControlEP<H> {
        let dest = hc.get_dest();

        ControlEP {
            logger: Logger::new(
                format!("dev{}-ep{}.usb.com", dest.dev, dest.ep),
                format!(
                    "\x1b[32mCO \x1b[34md{:02X}\x1b[35me{:02X}\x1b[m",
                    dest.dev, dest.ep
                ),
            ),
            mps: 8,
            toggle: 0,
            dest: dest.clone(),

            hc,
        }
    }

    pub fn set_data_toggle(&mut self, toggle: u8) {
        self.toggle = toggle;
    }

    pub fn set_max_packet_size(&mut self, mps: u8) {
        self.mps = mps;
        self.hc.set_max_packet_size(mps as u32);
    }

    pub fn send_setup(&mut self, req: StdRequest) -> TransactionResult<()> {
        let mut buf: [u8; 8] = req.into();

        let mut transaction = Transaction {
            token: TransactionToken::Setup,
            toggle: 0,
            buffer: &mut buf,
            length: 8,
        };

        if false {
            self.logger.info(format!("--> {transaction:?}"));
        }
        self.hc.submit_urb(&mut transaction)?;

        if false {
            self.logger.hex(LoggerLevel::Info, &buf, 8);
        }

        self.toggle = 1; // first toggle of data stage is always 1

        Ok(())
    }

    fn recv_packet(&mut self, buf: &mut [u8]) -> TransactionResult<()> {
        let length = buf.len();

        let mut transaction = Transaction {
            token: TransactionToken::In,
            toggle: self.toggle,
            buffer: buf,
            length: length as u8,
        };

        self.hc.submit_urb(&mut transaction)?;
        if false {
            self.logger.info(format!("<-- {transaction:?}"));
        }

        self.toggle = 1 - self.toggle;

        Ok(())
    }

    fn send_packet(&mut self, buf: &mut [u8]) -> TransactionResult<()> {
        let length = buf.len();
        let mut transaction = Transaction {
            token: TransactionToken::Out,
            toggle: self.toggle,
            buffer: buf,
            length: length as u8,
        };

        if false {
            self.logger.info(format!("--> {transaction:?}"));
        }
        self.hc.submit_urb(&mut transaction)?;

        self.toggle = 1 - self.toggle;

        Ok(())
    }

    pub fn send_packets(&mut self, buf: &mut [u8]) -> TransactionResult<()> {
        let mps: usize = self.mps.into();

        let length = buf.len();

        let chunks = length / mps;
        let remain = length % mps;

        for i in 0..chunks {
            let slice = &mut buf[i * mps..(i + 1) * mps];
            self.send_packet(slice)?;
        }

        if length == 0 || remain != 0 {
            let slice = &mut buf[length - remain..];
            self.send_packet(slice)?;
        }

        Ok(())
    }

    pub fn recv_packets(&mut self, buf: &mut [u8]) -> TransactionResult<()> {
        let mps: usize = self.mps.into();

        let length = buf.len();

        let chunks = length / mps;
        let remain = length % mps;

        for i in 0..chunks {
            let slice = &mut buf[i * mps..(i + 1) * mps];
            self.recv_packet(slice)?;
        }

        if length == 0 || remain != 0 {
            let slice = &mut buf[length - remain..];
            self.recv_packet(slice)?;
        }

        Ok(())
    }

    pub fn set_address(&mut self, new_address: u8) -> TransactionResult<()> {
        self.dest.dev = new_address;

        self.hc.get_dest_mut().dev = new_address;

        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::usb_core::hc::FakeHC;

    #[test]
    fn splitted_recv_bound_8() {
        let hc = Box::new(FakeHC::new(
            TransactionDestination { dev: 0, ep: 0 },
            EPType::Control,
            8,
        ));

        let mut ep = ControlEP::new(hc);

        let mut buf = [0; 8];
        let length = 8;

        ep.recv_packets(&mut buf, length).unwrap();

        assert_eq!(buf, [0, 1, 2, 3, 4, 5, 6, 7]);

        assert_eq!(ep.hc.request_fired, 1);
    }
}
