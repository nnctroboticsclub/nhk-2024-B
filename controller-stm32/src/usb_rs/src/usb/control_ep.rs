use alloc::format;

use crate::{logger::Logger, sleep_ms};

use super::{std_request::StdRequest, Transaction, TransactionDestination, TransactionToken, HC};

pub struct ControlEP {
    logger: Logger,

    mps: u8,    // max packet size
    toggle: u8, // data toggle
    dev: u8,    // device addr
    ep: u8,     // ep num
}

impl ControlEP {
    pub fn new(dev: u8, ep: u8) -> ControlEP {
        sleep_ms(500);

        ControlEP {
            logger: Logger::new(
                format!("dev{dev}-ep{ep}.usb.com"),
                format!("\x1b[32mCO \x1b[34md{dev:02X}\x1b[35me{ep:02X}\x1b[m"),
            ),
            mps: 8,
            toggle: 0,
            dev,
            ep,
        }
    }

    pub fn set_data_toggle(&mut self, toggle: u8) {
        self.toggle = toggle;
    }

    pub fn set_max_packet_size(&mut self, mps: u8) {
        self.mps = mps;
    }

    pub fn send_setup(&mut self, req: StdRequest) {
        let mut hc = HC::new(self.dev, self.ep);

        let mut buf: [u8; 8] = req.into();

        let mut transaction = Transaction {
            token: TransactionToken::Setup,
            dest: TransactionDestination {
                dev: self.dev,
                ep: self.ep,
            },
            toggle: 0,
            buffer: &mut buf,
            length: 8,
        };

        if false {
            self.logger.info(format!("--> {transaction:?}"));
        }
        hc.init(0, 0, 8);
        hc.submit_urb(&mut transaction);
        hc.wait_done();

        self.toggle = 1; // first toggle of data stage is always 1
    }

    fn recv_packet(&mut self, buf: &mut [u8], length: usize) {
        let mut hc = HC::new(self.dev, self.ep);

        let mut transaction = Transaction {
            token: TransactionToken::In,
            dest: TransactionDestination {
                dev: self.dev,
                ep: self.ep,
            },
            toggle: self.toggle,
            buffer: buf,
            length: length as u8,
        };

        hc.init(0, 0, 8);
        hc.submit_urb(&mut transaction);
        hc.wait_done();
        if false {
            self.logger.info(format!("<-- {transaction:?}"));
        }

        self.toggle = 1 - self.toggle;
    }

    fn send_packet(&mut self, buf: &mut [u8], length: usize) {
        let mut hc = HC::new(self.dev, self.ep);

        let mut transaction = Transaction {
            token: TransactionToken::Out,
            dest: TransactionDestination {
                dev: self.dev,
                ep: self.ep,
            },
            toggle: self.toggle,
            buffer: buf,
            length: length as u8,
        };

        if false {
            self.logger.info(format!("--> {transaction:?}"));
        }
        hc.init(0, 0, 8);
        hc.submit_urb(&mut transaction);
        hc.wait_done();

        self.toggle = 1 - self.toggle;
    }

    pub fn send_packets(&mut self, buf: &mut [u8], length: usize) {
        let mps = self.mps.into();

        let chunks = length / mps;
        let remain = length % mps;

        for i in 0..chunks {
            let slice = &mut buf[i * mps..(i + 1) * mps];
            self.send_packet(slice, mps);
        }

        let slice = &mut buf[length - remain..];
        self.send_packet(slice, remain);
    }

    pub fn recv_packets(&mut self, buf: &mut [u8], length: usize) {
        let mps = self.mps.into();

        let chunks = length / mps;
        let remain = length % mps;

        for i in 0..chunks {
            let slice = &mut buf[i * mps..(i + 1) * mps];
            self.recv_packet(slice, mps);
        }

        let slice = &mut buf[length - remain..];
        self.recv_packet(slice, remain);
    }
}
