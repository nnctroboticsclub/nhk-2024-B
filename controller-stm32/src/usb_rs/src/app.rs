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
use crate::usb_core::Endpoint;

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

    let mut ep2 = Endpoint::new(ep2_hc);
    let mut buf_ep2 = [0; 5];

    loop {
        if let Ok(()) = ep2.recv_packets(&mut buf_ep2) {
            logger.hex(LoggerLevel::Info, &buf_ep2, 5);
        };
        sleep_ms(10);
    }
}

fn cp210x() -> Result<(), Box<dyn core::error::Error>> {
    let mut logger = Logger::new("cp210x.nw", "  CP210x ");

    let ep0_hc = Box::new(BindedHC::new(
        super::usb_core::hc::TransactionDestination { dev: 1, ep: 0 },
        super::usb_core::hc::EPType::Control,
        8,
    ));

    let mut ep0 = USBEP0::new(ep0_hc);

    // Enable Interface
    logger.info("Enable Inteface");

    ep0.transaction(
        Direction::HostToDevice,
        RequestKind::Vendor,
        Recipient::Interface,
        RequestByte::CP210xIfcEnable,
        1,
        0,
        &mut [],
    )?;

    // Set baudrate to 9600
    logger.info("Set baudrate to 9600");
    ep0.transaction(
        Direction::HostToDevice,
        RequestKind::Vendor,
        Recipient::Interface,
        RequestByte::CP210xSetBaudRate,
        0,
        0,
        &mut [0x80, 0x25, 0x00, 0x00],
    )?;

    /*
     * XXYZ
     * X: Word Length (8)
     * Y: Parity (None = 0)
     * Z: Stop Bits (0f)
     */
    logger.info("Set Line ctrl 8N1");

    ep0.transaction(
        Direction::HostToDevice,
        RequestKind::Vendor,
        Recipient::Interface,
        RequestByte::CP210xSetLineCtrl,
        0x0800,
        0,
        &mut [],
    )?;

    /*
     * B5 B4 B3 B2 B1 B0
     * B5: 00 (EOF)
     * B4: 00 (Error)
     * B3: 00 (Break)
     * B2: 00 (Event)
     * B1: 11 (XON)
     * B0: 13 (XOFF)
     */
    logger.info("Set Special Characters");

    ep0.transaction(
        Direction::HostToDevice,
        RequestKind::Vendor,
        Recipient::Interface,
        RequestByte::CP210xSetChars,
        0,
        0,
        &mut [0x00, 0x00, 0x00, 0x00, 0x11, 0x13],
    )?;

    /*
    // AAAAAAAA BBBBBBBB CCCCCCCC DDDDDDDD
    // AAAAAAAA: ControlHandshake
    // BBBBBBBB: FlowReplace
    // CCCCCCCC: XonLimit
    // DDDDDDDD: XoffLimit

    logger.info("Get Flow");

    let mut buf = [0; 16];
    ep0.transaction(
        Direction::DeviceToHost,
        RequestKind::Vendor,
        Recipient::Interface,
        RequestByte::CP210xGetFlow,
        0,
        0,
        &mut buf,
    )?;

    let control_handshake = ((buf[0] as u32) << 24)
        | ((buf[1] as u32) << 16)
        | ((buf[2] as u32) << 8)
        | (buf[3] as u32);
    let flow_replace = ((buf[4] as u32) << 24)
        | ((buf[5] as u32) << 16)
        | ((buf[6] as u32) << 8)
        | (buf[7] as u32);
    let xon_limit = ((buf[8] as u32) << 24)
        | ((buf[9] as u32) << 16)
        | ((buf[10] as u32) << 8)
        | (buf[11] as u32);
    let xoff_limit = ((buf[12] as u32) << 24)
        | ((buf[13] as u32) << 16)
        | ((buf[14] as u32) << 8)
        | (buf[15] as u32);

    logger.info(format!("  Control Handshake: {:08X}", control_handshake));
    logger.info(format!("  Flow Replace: {:08X}", flow_replace));
    logger.info(format!("  Xon Limit: {:08X}", xon_limit));
    logger.info(format!("  Xoff Limit: {:08X}", xoff_limit));

    let flow_replace = 1;
    logger.info("  Flow Replace --> 1");

    logger.info("Set Flow");
    ep0.transaction(
        Direction::HostToDevice,
        RequestKind::Vendor,
        Recipient::Interface,
        RequestByte::CP210xSetFlow,
        0,
        0,
        &mut [
            (control_handshake >> 24) as u8,
            (control_handshake >> 16) as u8,
            (control_handshake >> 8) as u8,
            control_handshake as u8,
            (flow_replace >> 24) as u8,
            (flow_replace >> 16) as u8,
            (flow_replace >> 8) as u8,
            flow_replace as u8,
            (xon_limit >> 24) as u8,
            (xon_limit >> 16) as u8,
            (xon_limit >> 8) as u8,
            xon_limit as u8,
            (xoff_limit >> 24) as u8,
            (xoff_limit >> 16) as u8,
            (xoff_limit >> 8) as u8,
            xoff_limit as u8,
        ],
    )?; */

    // 000000ab 000000AB
    // a: DTR (mask)
    // b: RTS (mask)
    // A: DTR (state)
    // B: RTS (state)
    logger.info("Set MHS");
    ep0.transaction(
        Direction::HostToDevice,
        RequestKind::Vendor,
        Recipient::Interface,
        RequestByte::CP210xSetMHS,
        0x0303,
        0,
        &mut [],
    )?;

    /* logger.info("Get Communication Status");
    let mut buf = [0; 0x13];
    ep0.transaction(
        Direction::DeviceToHost,
        RequestKind::Vendor,
        Recipient::Interface,
        RequestByte::CP210xGetCommStatus,
        0,
        0,
        &mut buf,
    )?;
    logger.hex(LoggerLevel::Info, &buf, 0x13); */

    // Set baudrate to 19200
    logger.info("Set baudrate to 19200");

    ep0.transaction(
        Direction::HostToDevice,
        RequestKind::Vendor,
        Recipient::Interface,
        RequestByte::CP210xSetBaudRate,
        0,
        0,
        &mut [0x00, 0x4b, 0x00, 0x00],
    )?;

    // 000000ab 000000AB
    // a: DTR (mask)
    // b: RTS (mask)
    // A: DTR (state)
    // B: RTS (state)
    logger.info("Set MHS");
    ep0.transaction(
        Direction::HostToDevice,
        RequestKind::Vendor,
        Recipient::Interface,
        RequestByte::CP210xSetMHS,
        0x0300,
        0,
        &mut [],
    )?;

    // (00 0x)16
    // x: (ABCD)2
    // bit 0: Clear the transmit queue.
    // bit 1: Clear the receive queue.
    // bit 2: Clear the transmit queue.
    // bit 3: Clear the receive queue.
    logger.info("Purge");
    ep0.transaction(
        Direction::HostToDevice,
        RequestKind::Vendor,
        Recipient::Interface,
        RequestByte::CP210xPurge,
        0x0003,
        0,
        &mut [],
    )?;

    sleep_ms(100);

    // Send 'RDID\r\n'
    // Endpoint [0x02] OUT Bulk
    logger.info("Send 'RDID\\r\\n'");
    {
        let ep2_hc = Box::new(BindedHC::new(
            super::usb_core::hc::TransactionDestination { dev: 1, ep: 2 },
            super::usb_core::hc::EPType::Bulk,
            64,
        ));

        let mut ep2 = Endpoint::new(ep2_hc);

        ep2.send_packets(&mut [0x52, 0x44, 0x49, 0x44, 0x0D, 0x0A])?;
    }

    // recv forever
    logger.info("Recv forever");

    let mut buf = [0; 64];
    let ep82_hc = Box::new(BindedHC::new(
        super::usb_core::hc::TransactionDestination { dev: 1, ep: 0x82 },
        super::usb_core::hc::EPType::Bulk,
        64,
    ));

    let mut ep82 = Endpoint::new(ep82_hc);

    loop {
        sleep_ms(1000);

        buf.fill(0x5A);
        let result = ep82.recv_packets(&mut buf);
        log(format!("Result: {:?}", result));
        if result.is_err() {
            continue;
        }

        logger.hex(LoggerLevel::Info, &buf, 64);
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

    cp210x()?;

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
