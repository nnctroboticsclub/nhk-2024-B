use alloc::boxed::Box;

use crate::logger::LoggerLevel;
use crate::usb::HIDClassDescriptor;
use crate::usb_core::hc::HC;
use crate::usb_core::std_request::Direction;
use crate::usb_core::std_request::Recipient;
use crate::usb_core::std_request::RequestByte;
use crate::usb_core::std_request::RequestKind;
use crate::usb_core::std_request::RequestType;
use crate::usb_core::std_request::StdRequest;
use crate::usb_core::ControlEP;

use super::alloc::format;
use super::alloc::vec;
use super::common::log;
use super::common::sleep_ms;
use super::logger::Logger;
use super::usb::ConfigurationDescriptor;
use super::usb::Descriptor;
use super::usb::DeviceDescriptor;
use super::usb::NewDescriptor;
use super::usb::ParsingContext;
use super::usb::PhysicalEP0;
use super::usb::EP0;
use super::usb::USBEP0;
use super::usb_core::hc::BindedHC;
use super::usb_core::hcd::BindedHcd;
use super::usb_core::hcd::Hcd;

fn enumerate(hc: Box<BindedHC>) -> Result<ConfigurationDescriptor, Box<dyn core::error::Error>> {
    let mut logger = Logger::new("enum.usb.com", "Enumerate");
    let mut ep0 = USBEP0::new(hc);
    ep0.set_address(1)?;

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

    // get configuration descriptor
    let total_size = {
        let mut buf = [0; 4];
        ctx.ep0.get_descriptor(0x02, 0, &mut buf)?;
        (buf[3] as u16) << 8 | (buf[2] as u16)
    };

    let mut buf = vec![0; total_size as usize];
    ctx.ep0.get_descriptor(0x02, 0, buf.as_mut_slice())?;

    let (_, config) = ConfigurationDescriptor::parse(&mut ctx, buf.as_mut_slice()).unwrap();

    logger.info(format!(
        "Configuration Descriptor[{}: {}]:",
        config.id, config.name
    ));
    logger.info(format!("- Attributes: {}", config.attributes));
    logger.info(format!("- Max Power: {}", config.max_power));
    for interface in &config.interfaces {
        logger.info(format!(
            "- Interface [{}: {}]",
            interface.id, interface.name
        ));
        logger.info(format!(
            "  - Kind: {}-{}-{}",
            interface.if_class, interface.if_subclass, interface.if_proto
        ));
        logger.info(format!("  - Alt Setting: {}", interface.alt_setting));
        if interface.hid_desc.is_some() {
            let desc = interface.hid_desc.clone().unwrap();
            logger.info(format!("  - HID"));
            logger.info(format!("    - hid_bcd: {:04x}", desc.bcd_hid));
            logger.info(format!("    - country_code: {}", desc.country_code));
            logger.info(format!("    - class_descriptors"));
            for (i, cd) in desc.class_descriptors.iter().enumerate() {
                logger.info(format!("      - {:?}", cd));

                let length = match cd {
                    HIDClassDescriptor::Report { size } => size,
                    HIDClassDescriptor::Physical { size } => size,
                    HIDClassDescriptor::Unknown { kind, size } => size,
                };

                let desc_type = match cd {
                    HIDClassDescriptor::Report { size: _ } => 0x22,
                    HIDClassDescriptor::Physical { size: _ } => 0x23,
                    HIDClassDescriptor::Unknown { kind, size: _ } => {
                        logger.info(format!("Unknown HID descriptor kind: {}", kind));
                        continue;
                    }
                };

                let mut buf = vec![0; *length as usize];
                ep0.transaction(
                    Direction::DeviceToHost,
                    RequestKind::Standard,
                    Recipient::Interface,
                    RequestByte::GetDescriptor,
                    (desc_type << 8 | i).try_into().unwrap(), // report descriptor type: desc_type, index: i
                    interface.id as u16,
                    &mut buf,
                )?;

                logger.hex(LoggerLevel::Info, buf.as_mut(), *length as i32);
            }
        }
        logger.info(format!("  - Endpoints:"));
        for ep in &interface.endpoints {
            logger.info(format!(
                "    - EP[{:02x}]: attr: {}, mps: {}, interval: {}",
                ep.address, ep.attributes, ep.mps, ep.interval
            ));
        }
    }

    Ok(config)
}

fn run() -> Result<(), Box<dyn core::error::Error>> {
    let mut logger = Logger::new("usb.com", "RUST CODE");

    logger.info("Rust code started");

    {
        let mut hcd = BindedHcd::new();
        hcd.init();
        sleep_ms(200);

        hcd.wait_device();
        sleep_ms(200);

        hcd.reset_port();
        sleep_ms(100);
    }

    let hc = Box::new(BindedHC::new(
        super::usb_core::hc::TransactionDestination { dev: 0, ep: 0 },
        super::usb_core::hc::EPType::Control,
        8,
    ));
    let _config = enumerate(hc)?;

    /* let interface = config.interfaces.iter().find(|x| x.hid_desc.is_some());
    if interface.is_none() {
        return Err("No HID interface found".into());
    }

    let interface = interface.unwrap(); */

    if false {
        let hc = Box::new(BindedHC::new(
            super::usb_core::hc::TransactionDestination { dev: 1, ep: 0 },
            super::usb_core::hc::EPType::Control,
            8,
        ));
        let mut ep0 = USBEP0::new(hc);

        let mut buf = [0; 32];
        let mut failed_count = 0;

        loop {
            let ret = ep0.transaction(
                Direction::DeviceToHost,
                RequestKind::Class,
                Recipient::Interface,
                RequestByte::HidGetReport,
                0x0101, // report type, report id
                1,      // interface
                &mut buf,
            );

            if ret.is_ok() {
                logger.hex(LoggerLevel::Info, &buf, 32);
            }

            if ret.is_err() {
                failed_count += 1;
                if failed_count > 10 {
                    log(format!("Error: {:?}", ret));

                    failed_count = 0;
                }
            }

            sleep_ms(500);
        }
    } else {
        let hc = Box::new(BindedHC::new(
            super::usb_core::hc::TransactionDestination { dev: 1, ep: 0x2 },
            super::usb_core::hc::EPType::Interrupt,
            7,
        ));

        let mut ep = ControlEP::new(hc);
        let mut buf = [0; 7];

        loop {
            let ret = ep.recv_packets(&mut buf);

            if ret.is_ok() {
                logger.hex(LoggerLevel::Info, &buf, 7);
            }

            if ret.is_err() {
                log(format!("Error: {:?}", ret));
            }

            sleep_ms(200);
        }
    }

    Ok(())
}

#[no_mangle]
pub extern "C" fn usb_rs_run() {
    let res = run();
    if res.is_err() {
        log(format!("Error: {:?}", res.err()));
    }

    log("Rust code finished");
}
