#![no_std]

mod binding;
mod binding_basic;
mod binding_logger;

mod allocator;
mod logger;
mod usb;

extern crate alloc;

use alloc::format;
use binding::stm32_usb_host_UrbStatus_kDone;
use logger::Logger;
use logger::LoggerLevel;
use usb::std_request;
use usb::std_request::StdRequest;
use usb::Hcd;
use usb::RawURBStatus;
use usb::HC;

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}

fn sleep_ms(ms: i32) {
    unsafe {
        binding_basic::sleep_ms(ms);
    }
}

struct ControlEP {
    logger: Logger,

    toggle: u8,
    dev: u8,
    ep: u8,
}

impl ControlEP {
    pub fn new(dev: u8, ep: u8) -> ControlEP {
        sleep_ms(500);

        ControlEP {
            logger: Logger::new(
                format!("dev{dev}-ep{ep}.usb.com"),
                format!("\x1b[32mCO \x1b[34md{dev:02X}\x1b[35me{ep:02X}\x1b[m"),
            ),
            toggle: 0,
            dev: dev,
            ep: ep,
        }
    }

    pub fn set_data_toggle(&mut self, toggle: u8) {
        self.toggle = toggle;
    }

    pub fn send_setup(&mut self, req: StdRequest) {
        let mut hc = HC::new(self.dev, self.ep);

        let mut buf: [u8; 8] = req.into();

        let mut transaction = usb::Transaction {
            token: usb::TransactionToken::Setup,
            dest: usb::TransactionDestination {
                dev: self.dev,
                ep: self.ep,
            },
            toggle: 0,
            buffer: &mut buf,
            length: 8,
        };

        self.logger.info(format!("--> {:?}", &transaction));

        hc.init(0, 0, 8);
        hc.submit_urb(&mut transaction);
        hc.wait_done();

        self.toggle = 1; // first toggle of data stage is always 1
    }

    pub fn recv_packet(&mut self, buf: &mut [u8], length: i32) {
        let mut hc = HC::new(self.dev, self.ep);

        let mut recv_buf: [u8; 8] = [0x5A; 8];
        let mut transaction = usb::Transaction {
            token: usb::TransactionToken::In,
            dest: usb::TransactionDestination {
                dev: self.dev,
                ep: self.ep,
            },
            toggle: self.toggle,
            buffer: &mut recv_buf,
            length: length as u8,
        };

        hc.init(0, 0, 8);
        hc.submit_urb(&mut transaction);
        sleep_ms(100);
        hc.wait_done();

        self.logger.info(format!("<-- {:?}", &transaction));

        if length != 0 {
            buf.copy_from_slice(&recv_buf[0..length as usize]);
        }

        self.toggle = 1 - self.toggle;
    }

    pub fn send_packet(&mut self, buf: &mut [u8], length: i32) {
        let mut hc = HC::new(self.dev, self.ep);

        let mut transaction = usb::Transaction {
            token: usb::TransactionToken::Out,
            dest: usb::TransactionDestination {
                dev: self.dev,
                ep: self.ep,
            },
            toggle: self.toggle,
            buffer: buf,
            length: length as u8,
        };

        self.logger.info(format!("--> {:?}", &transaction));

        hc.init(0, 0, 8);
        hc.submit_urb(&mut transaction);
        sleep_ms(100);

        self.toggle = 1 - self.toggle;
    }
}

fn run() -> () {
    let mut logger = Logger::new("usb.com", "RUST CODE");

    logger.info("Rust code started");

    let mut hcd = Hcd::new();
    hcd.init();

    hcd.wait_device();
    sleep_ms(100);

    hcd.reset_port();
    sleep_ms(100);

    let mut ep = ControlEP::new(0, 0);
    let mut buf: [u8; 8] = [0; 8];

    ep.send_setup(StdRequest {
        request_type: std_request::RequestType {
            direction: std_request::Direction::HostToDevice,
            req_type: std_request::RequestKind::Standard,
            recipient: std_request::Recipient::Device,
        },
        request: std_request::RequestByte::SetAddress,
        value: 0x00_01, // dev 1
        index: 0x00_00,
        length: 0x00,
    });

    ep.recv_packet(&mut buf, 0);

    logger.info("Starting transaction with new address [1]");
    let mut ep = ControlEP::new(1, 0);
    logger.info(format!(
        "\x1b[44m========================================\x1b[m"
    ));

    ep.send_setup(StdRequest {
        request_type: std_request::RequestType {
            direction: std_request::Direction::DeviceToHost,
            req_type: std_request::RequestKind::Standard,
            recipient: std_request::Recipient::Device,
        },
        request: std_request::RequestByte::GetDescriptor,
        value: 0x01_00, // 1: Device
        index: 0x00_00,
        length: 0x12,
    });

    ep.recv_packet(&mut buf, 8);
    ep.recv_packet(&mut buf, 8);
    ep.recv_packet(&mut buf, 2);
    ep.send_packet(&mut buf, 0);

    /* ep.send_setup(StdRequest {
        request_type: std_request::RequestType {
            direction: std_request::Direction::DeviceToHost,
            req_type: std_request::RequestKind::Standard,
            recipient: std_request::Recipient::Device,
        },
        request: std_request::RequestByte::GetDescriptor,
        value: 0x02_00, // 1: Device
        index: 0x00_00,
        length: 0x0012,
    });

    ep.recv_packet(&mut buf, 8);
    ep.recv_packet(&mut buf, 8);

    ep.recv_packet(&mut buf, 8);
    ep.recv_packet(&mut buf, 8);

    ep.recv_packet(&mut buf, 8);
    ep.recv_packet(&mut buf, 8);

    ep.recv_packet(&mut buf, 3); */
}

#[no_mangle]
pub extern "C" fn usb_rs_run() {
    run();
}
