#![no_std]

mod binding;
mod binding_basic;
mod binding_logger;

mod allocator;
mod logger;
mod usb;

extern crate alloc;

use alloc::format;
use logger::Logger;
use logger::LoggerLevel;
use usb::std_request;
use usb::std_request::StdRequest;
use usb::Hcd;
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

    dev: u8,
    ep: u8,

    data_toggle_out: bool,
}

impl ControlEP {
    pub fn new(dev: u8, ep: u8) -> ControlEP {
        ControlEP {
            logger: Logger::new(
                format!("dev{dev}-ep{ep}.usb.com"),
                format!("CO d{dev:02X}e{ep:02X}"),
            ),
            dev,
            ep,
            data_toggle_out: false,
        }
    }

    pub fn send_setup(&mut self, req: StdRequest) {
        let mut hc = HC::new();
        let mut buf: [u8; 8] = req.into();

        self.logger.info("Sending request");
        self.logger.hex(LoggerLevel::Info, &buf, 8);

        hc.init(self.ep as i32, self.dev as i32, 1, 0, 0x08);
        self.logger
            .info(format!("Using Data toggle: {:?}", self.data_toggle_out));
        hc.data01(self.data_toggle_out);

        hc.submit_urb(false, &mut buf, 8, true, false);
        hc.wait_urb_done();

        self.data_toggle_out = !self.data_toggle_out;
    }

    pub fn recv_packet(&mut self, buf: &mut [u8], length: i32) {
        // Initialize buffer
        for i in 0..length {
            buf[i as usize] = 0x55u8;
        }

        let mut hc = HC::new();
        hc.init(self.ep as i32, self.dev as i32, 1, 0, 0x08);

        hc.data01(self.data_toggle_out);
        hc.submit_urb(true, buf, length, false, false);
        hc.wait_urb_done();
        sleep_ms(100);

        let status = hc.status();
        self.logger
            .info(format!("Data received. Status: {:?}", status));
        self.logger.hex(LoggerLevel::Info, buf, length);

        self.data_toggle_out = !self.data_toggle_out;
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

    let mut buf: [u8; 8] = [0; 8];
    ep.recv_packet(&mut buf, 8);
    ep.recv_packet(&mut buf, 8);

    ep.recv_packet(&mut buf, 2);
}

#[no_mangle]
pub extern "C" fn usb_rs_run() {
    run();
}
