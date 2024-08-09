use alloc::boxed::Box;

use crate::logger::LoggerLevel;
use crate::usb::HIDClassDescriptor;
use crate::usb_core::hc::TransactionError;
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

    ep0.transaction(
        Direction::HostToDevice,
        RequestKind::Standard,
        Recipient::Device,
        RequestByte::SetConfiguration,
        1,
        0,
        &mut [],
    )?;

    Ok(config)
}

fn loop_mouse() -> Result<(), Box<dyn core::error::Error>> {
    let mut logger = Logger::new("usb.com", "RUST CODE");

    let ep2_hc = Box::new(BindedHC::new(
        super::usb_core::hc::TransactionDestination { dev: 1, ep: 0x82 },
        super::usb_core::hc::EPType::Interrupt,
        7,
    ));

    let mut ep2 = ControlEP::new(ep2_hc);
    let mut buf_ep2 = [0; 5];

    loop {
        if let Ok(()) = ep2.recv_packets(&mut buf_ep2) {
            logger.hex(LoggerLevel::Info, &buf_ep2, 5);
        };
        sleep_ms(10);
    }
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

    // loop_mouse()?;

    /* let result = ep0.send_setup(StdRequest {
        request_type: RequestType {
            direction: Direction::DeviceToHost,
            req_type: RequestKind::Class,
            recipient: Recipient::Interface,
        },
        request: RequestByte::HidGetReport,
        value: 0x02_00, // report type: input, report id: 0
        index: 0,       // interface index
        length: 1,
    })?;

    buf.fill(0x5A);
    ep0.recv_packets(&mut buf)?;
    ep0.send_packets(&mut [])?; */

    let ep0_hc = Box::new(BindedHC::new(
        super::usb_core::hc::TransactionDestination { dev: 1, ep: 0 },
        super::usb_core::hc::EPType::Control,
        8,
    ));

    let mut ep0 = ControlEP::new(ep0_hc);

    let mut buf = [0; 16];

    loop {
        sleep_ms(50);

        let result = ep0.send_setup(StdRequest {
            request_type: RequestType {
                direction: Direction::DeviceToHost,
                req_type: RequestKind::Class,
                recipient: Recipient::Interface,
            },
            request: RequestByte::HidGetReport,
            value: 0x01_00, // report type: input, report id: 0
            index: 0,       // interface index
            length: 8,
        });
        if result.is_err() {
            continue;
        }

        buf.fill(0x5A);
        let result = ep0.recv_packets(&mut buf[0..8]);
        if result.is_err() {
            continue;
        }

        let result = ep0.send_packets(&mut []);
        if result.is_err() {
            continue;
        }

        logger.hex(LoggerLevel::Info, &buf, 16);
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
