#![no_std]
#![feature(str_from_utf16_endian)]

mod binding;
mod binding_basic;
mod binding_logger;

mod allocator;
mod common;
mod logger;
mod usb;
mod usb_core;

extern crate alloc;

use alloc::format;
use alloc::vec;
use common::sleep_ms;
use logger::Logger;
use usb::ConfigurationDescriptor;
use usb::Descriptor;
use usb::DeviceDescriptor;
use usb::NewDescriptor;
use usb::ParsingContext;
use usb::EP0;
use usb::USBEP0;
use usb_core::Hcd;

fn run() -> () {
    let mut logger = Logger::new("usb.com", "RUST CODE");

    logger.info("Rust code started");

    {
        let mut hcd = Hcd::new();
        hcd.init();
        hcd.wait_device();

        hcd.reset_port();
        sleep_ms(100);
    }

    let mut ep0 = USBEP0::new(0);

    // ep0.set_address(1);

    {
        let mut ctx = ParsingContext { ep0: &mut ep0 };

        // get device descriptor
        let desc = DeviceDescriptor::new(&mut ctx, 0).unwrap();

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
        let mut ctx = ParsingContext { ep0: &mut ep0 };

        let total_size = {
            let mut buf = [0; 4];
            ctx.ep0.get_descriptor(0x02, 0, &mut buf, 0x4);
            (buf[3] as u16) << 8 | (buf[2] as u16)
        };

        let mut buf = vec![0; total_size as usize];
        ctx.ep0
            .get_descriptor(0x02, 0, buf.as_mut_slice(), total_size);
        logger.hex(
            logger::LoggerLevel::Info,
            buf.as_mut_slice(),
            total_size.into(),
        );

        let _config = ConfigurationDescriptor::parse(&mut ctx, buf.as_mut_slice());
    }
}

#[no_mangle]
pub extern "C" fn usb_rs_run() {
    run();
}
