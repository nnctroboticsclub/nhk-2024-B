use alloc::format;

use crate::logger::Logger;

use super::{
    hc::{EPType, Transaction, TransactionDestination, TransactionResult, TransactionToken, HC},
    std_request::StdRequest,
};

pub struct ControlEP<H: HC> {
    logger: Logger,
    hc: H,

    dest: TransactionDestination,
    mps: u8,
    toggle: u8,
}

impl<H: HC> ControlEP<H> {
    pub fn new(dev: u8, ep: u8) -> ControlEP<H> {
        let dest = TransactionDestination { dev, ep };

        ControlEP {
            logger: Logger::new(
                format!("dev{dev}-ep{ep}.usb.com"),
                format!("\x1b[32mCO \x1b[34md{dev:02X}\x1b[35me{ep:02X}\x1b[m"),
            ),
            mps: 8,
            toggle: 0,
            dest,

            hc: H::new(dest, EPType::Control, 8),
        }
    }

    pub fn set_data_toggle(&mut self, toggle: u8) {
        self.toggle = toggle;
    }

    pub fn set_max_packet_size(&mut self, mps: u8) {
        self.mps = mps;
    }

    pub fn send_setup(&mut self, req: StdRequest) -> TransactionResult {
        let mut hc = H::new(self.dest, EPType::Control, self.mps as i32);

        let mut buf: [u8; 8] = req.into();

        let mut transaction = Transaction {
            token: TransactionToken::Setup,
            toggle: 0,
            buffer: &mut buf,
            length: 8,
        };

        if true {
            self.logger.info(format!("--> {transaction:?}"));
        }
        hc.submit_urb(&mut transaction)?;

        self.toggle = 1; // first toggle of data stage is always 1

        Ok(())
    }

    fn recv_packet(&mut self, buf: &mut [u8], length: usize) -> TransactionResult {
        let mut hc = H::new(self.dest, EPType::Control, self.mps as i32);

        let mut transaction = Transaction {
            token: TransactionToken::In,
            toggle: self.toggle,
            buffer: buf,
            length: length as u8,
        };

        hc.submit_urb(&mut transaction)?;
        if true {
            self.logger.info(format!("<-- {transaction:?}"));
        }

        self.toggle = 1 - self.toggle;

        Ok(())
    }

    fn send_packet(&mut self, buf: &mut [u8], length: usize) -> TransactionResult {
        let mut hc = H::new(self.dest, EPType::Control, self.mps as i32);

        let mut transaction = Transaction {
            token: TransactionToken::Out,
            toggle: self.toggle,
            buffer: buf,
            length: length as u8,
        };

        if true {
            self.logger.info(format!("--> {transaction:?}"));
        }
        hc.submit_urb(&mut transaction)?;

        self.toggle = 1 - self.toggle;

        Ok(())
    }

    pub fn send_packets(&mut self, buf: &mut [u8], length: usize) -> TransactionResult {
        let mps = self.mps.into();

        let chunks = length / mps;
        let remain = length % mps;

        for i in 0..chunks {
            let slice = &mut buf[i * mps..(i + 1) * mps];
            self.send_packet(slice, mps)?;
        }

        let slice = &mut buf[length - remain..];
        self.send_packet(slice, remain)?;

        Ok(())
    }

    pub fn recv_packets(&mut self, buf: &mut [u8], length: usize) -> TransactionResult {
        let mps = self.mps.into();

        let chunks = length / mps;
        let remain = length % mps;

        for i in 0..chunks {
            let slice = &mut buf[i * mps..(i + 1) * mps];
            self.recv_packet(slice, mps)?;
        }

        let slice = &mut buf[length - remain..];
        self.recv_packet(slice, remain)?;

        Ok(())
    }
}
