#![no_std]
#![feature(str_from_utf16_endian)]

mod binding;
mod binding_basic;
mod binding_logger;

mod allocator;
mod logger;
mod usb;

extern crate alloc;

use alloc::format;
use alloc::string::String;
use alloc::string::ToString;
use logger::Logger;
use usb::std_request;
use usb::std_request::StdRequest;
use usb::ControlEP;
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

struct EP0 {
    ep: ControlEP,
    dev: u8,
}

impl EP0 {
    pub fn new(dev: u8) -> EP0 {
        EP0 {
            ep: ControlEP::new(dev, 0),
            dev,
        }
    }

    pub fn set_max_packet_size(&mut self, mps: u8) {
        self.ep.set_max_packet_size(mps);
    }

    fn transaction(
        &mut self,
        direction: std_request::Direction,
        req_type: std_request::RequestKind,
        recipient: std_request::Recipient,
        request: std_request::RequestByte,
        value: u16,
        index: u16,
        buf: &mut [u8],
        length: u16,
    ) {
        use std_request::Direction;
        let is_out = direction == Direction::HostToDevice;

        self.ep.send_setup(StdRequest {
            request_type: std_request::RequestType {
                direction,
                req_type,
                recipient,
            },
            request,
            value: value,
            index: index,
            length: length,
        });

        if is_out {
            self.ep.send_packets(buf, length.into());
            self.ep.set_data_toggle(1);
            self.ep.recv_packets(buf, 0);
        } else {
            self.ep.recv_packets(buf, length.into());
            self.ep.set_data_toggle(1);
            self.ep.send_packets(buf, 0);
        }
    }

    pub fn set_address(&mut self, new_address: u8) {
        let mut buf = [0u8; 0];
        self.transaction(
            std_request::Direction::HostToDevice,
            std_request::RequestKind::Standard,
            std_request::Recipient::Device,
            std_request::RequestByte::SetAddress,
            new_address.into(), // value = addr
            0x00_00,            // index = 0
            &mut buf,           // buffer (dummy)
            0,                  // length = 0
        );

        self.dev = new_address;
        self.ep = ControlEP::new(self.dev, 0);
    }

    pub fn get_descriptor(&mut self, descriptor_type: u8, index: u8, buf: &mut [u8], length: u16) {
        let value = (descriptor_type as u16) << 8 | (index as u16);
        let value = value.into();

        self.transaction(
            std_request::Direction::DeviceToHost,
            std_request::RequestKind::Standard,
            std_request::Recipient::Device,
            std_request::RequestByte::GetDescriptor,
            value,
            0x00_00,
            buf,
            length,
        );
    }

    pub fn get_string(&mut self, index: u8, buf: &mut [u8], max_length: u16) -> String {
        self.get_descriptor(3, index, buf, 2);
        let length = buf[0] as usize;

        self.get_descriptor(3, index, buf, length.try_into().unwrap());

        let v = &buf[0..length];
        return String::from_utf16le_lossy(v);
    }
}

struct DeviceDescriptor {
    // length
    // type
    bcd_usb: u16,
    usb_class: u8,
    usb_subclass: u8,
    usb_proto: u8,
    mps: u8,

    vid: u16,
    pid: u16,
    bcd: u16,
    manufacturer: String,
    product: String,

    serial: String,
    num_configurations: u8,
}

impl DeviceDescriptor {
    pub fn new(ep0: &mut EP0, buf: &mut [u8], max_length: u16) -> DeviceDescriptor {
        ep0.get_descriptor(0x01, 0, buf, 0x8);
        let device_descriptor_size = buf[0];
        let bcd_usb = (buf[2] as u16) << 8 | (buf[3] as u16);
        let usb_class = buf[4];
        let usb_subclass = buf[5];
        let usb_proto = buf[6];
        let mps = buf[7];
        ep0.set_max_packet_size(mps);

        ep0.get_descriptor(0x01, 0, buf, 0x12);
        let vid = (buf[8] as u16) << 8 | (buf[9] as u16);
        let pid = (buf[10] as u16) << 8 | (buf[11] as u16);
        let bcd = (buf[12] as u16) << 8 | (buf[13] as u16);
        let i_manufacturer = buf[14];
        let i_product = buf[15];
        let i_serial = buf[16];
        let num_configurations = buf[17];

        let manufacturer = if i_manufacturer != 0 {
            ep0.get_string(i_manufacturer, buf, max_length)
        } else {
            "-----".to_string()
        };

        let product = if i_product != 0 {
            ep0.get_string(i_product, buf, max_length)
        } else {
            "-----".to_string()
        };

        let serial = if i_serial != 0 {
            ep0.get_string(i_serial, buf, max_length)
        } else {
            "-----".to_string()
        };

        DeviceDescriptor {
            bcd_usb,
            usb_class,
            usb_subclass,
            usb_proto,
            mps,
            vid,
            pid,
            bcd,
            manufacturer,
            product,
            serial,
            num_configurations,
        }
    }
}

fn run() -> () {
    let mut logger = Logger::new("usb.com", "RUST CODE");

    logger.info("Rust code started");

    let mut hcd = Hcd::new();
    hcd.init();

    hcd.wait_device();

    hcd.reset_port();

    // let mut ep = ControlEP::new(0, 0);
    let mut ep0 = EP0::new(0);
    let mut buf: [u8; 0x100] = [0; 0x100];

    ep0.set_address(1);

    {
        // get device descriptor
        let desc = DeviceDescriptor::new(&mut ep0, &mut buf, 0x100);

        logger.info(format!("Device Information"));
        logger.info(format!(
            "  Class       : {}-{}-{}",
            desc.usb_class, desc.usb_subclass, desc.usb_proto
        ));
        logger.info(format!("  VID, PID    : {:04X}-{:04X}", desc.vid, desc.pid));
        logger.info(format!("  Configs     : {}", desc.num_configurations));

        logger.info(format!("  Manufacturer: {}", desc.manufacturer));
        logger.info(format!("  Product     : {}", desc.product));
        logger.info(format!("  Serial      : {}", desc.serial));
    }

    {
        ep0.get_descriptor(0x02, 0, &mut buf, 0x9);
        let total_size = (buf[3] as u16) << 8 | (buf[2] as u16);

        ep0.get_descriptor(0x02, 0, &mut buf, total_size);
        logger.hex(logger::LoggerLevel::Info, &buf, total_size.into());
    }
}

#[no_mangle]
pub extern "C" fn usb_rs_run() {
    run();
}
