#![no_std]
#![feature(str_from_utf16_endian)]

mod binding;
mod binding_basic;
mod binding_logger;

mod allocator;
mod common;
mod logger;
mod usb;

extern crate alloc;

use core::fmt::Display;

use alloc::borrow::Cow;
use alloc::boxed::Box;
use alloc::ffi::CString;
use alloc::format;
use alloc::string::String;
use alloc::string::ToString;
use alloc::vec;
use alloc::vec::Vec;
use common::log;
use common::sleep_ms;
use logger::Logger;
use nom::bytes::complete::tag;
use nom::multi::count;
use nom::number::complete::le_u16;
use nom::number::complete::le_u8;
use nom::IResult as NomResult;
use usb::std_request;
use usb::std_request::StdRequest;
use usb::ControlEP;
use usb::Hcd;

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

    pub fn get_string(&mut self, index: u8) -> String {
        let length = {
            let buf = &mut [0; 2];
            self.get_descriptor(3, index, buf, 2);
            buf[0] as usize
        };

        let mut buf = vec![0; length];

        self.get_descriptor(3, index, buf.as_mut_slice(), length.try_into().unwrap());

        let v = &buf[0..length];
        return String::from_utf16le_lossy(v);
    }
}

struct ParsingContext<'a> {
    pub ep0: &'a mut EP0,
}

struct UsbString(u8);

impl UsbString {
    fn new(id: u8) -> UsbString {
        UsbString(id)
    }

    fn read(&self, ctx: &mut ParsingContext) -> String {
        if self.0 != 0 {
            log(format!("Using Normal getstring {}", self.0));
            ctx.ep0.get_string(self.0)
        } else {
            log(format!("fallback ({})", self.0));
            "-----".to_string()
        }
    }
}

trait Descriptor: Sized {
    fn get_type() -> u8;
    fn get_length() -> Option<usize>;

    fn parse<'a>(ctx: &mut ParsingContext, buf: &'a [u8]) -> NomResult<&'a [u8], Self>;

    fn new(ctx: &mut ParsingContext, index: u8) -> Option<Self> {
        let length: u16 = Self::get_length()?.try_into().ok()?;

        let mut buf = vec![0; length as usize];

        ctx.ep0
            .get_descriptor(Self::get_type(), index, buf.as_mut_slice(), length);

        Self::parse(ctx, buf.as_mut_slice()).map(|x| x.1).ok()
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

impl Descriptor for DeviceDescriptor {
    fn get_type() -> u8 {
        1
    }

    fn get_length() -> Option<usize> {
        Some(12)
    }

    fn parse<'a>(ctx: &mut ParsingContext, input: &'a [u8]) -> NomResult<&'a [u8], Self> {
        let (input, _) = le_u8(input)?;
        let (input, _) = tag("\x01")(input)?; // type
        let (input, bcd_usb) = le_u16(input)?;
        let (input, usb_class) = le_u8(input)?;
        let (input, usb_subclass) = le_u8(input)?;
        let (input, usb_proto) = le_u8(input)?;
        let (input, mps) = le_u8(input)?;
        let (input, vid) = le_u16(input)?;
        let (input, pid) = le_u16(input)?;
        let (input, bcd) = le_u16(input)?;
        let (input, i_manufacturer) = le_u8(input)?;
        let (input, i_product) = le_u8(input)?;
        let (input, i_serial) = le_u8(input)?;
        let (input, num_configurations) = le_u8(input)?;

        let manufacturer = UsbString::new(i_manufacturer).read(ctx);
        let product = UsbString::new(i_product).read(ctx);
        let serial = UsbString::new(i_serial).read(ctx);

        Ok((
            input,
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
            },
        ))
    }

    fn new(ctx: &mut ParsingContext, index: u8) -> Option<Self> {
        let mps = {
            let buf = &mut [0; 8];
            ctx.ep0.get_descriptor(1, index, buf, 8);
            buf[7]
        };
        ctx.ep0.set_max_packet_size(mps);

        // usually object construction
        let buf = &mut [0; 0x12];
        ctx.ep0.get_descriptor(1, index, buf, 0x12);
        Self::parse(ctx, buf).map(|x| x.1).ok()
    }
}

#[derive(Debug)]
struct InterfaceDescriptor {
    id: u8,
    if_class: u8,
    if_subclass: u8,
    if_proto: u8,

    interface: String,

    alt_setting: u8,
    num_endpoints: u8,
}

impl Descriptor for InterfaceDescriptor {
    fn get_type() -> u8 {
        4
    }
    fn get_length() -> Option<usize> {
        Some(9)
    }

    fn parse<'a>(ctx: &mut ParsingContext, input: &'a [u8]) -> NomResult<&'a [u8], Self> {
        log(format!(
            "InterfaceDescriptor::parse ({})",
            input[0..2]
                .iter()
                .map(|x| format!("{:02X}", x))
                .collect::<String>()
        ));
        let (input, _) = le_u8(input)?; // length
        let (input, _) = le_u8(input)?; // type
        let (input, id) = le_u8(input)?;
        let (input, alt_setting) = le_u8(input)?;
        let (input, num_endpoints) = le_u8(input)?;
        let (input, if_class) = le_u8(input)?;
        let (input, if_subclass) = le_u8(input)?;
        let (input, if_proto) = le_u8(input)?;
        let (input, interface_string_id) = le_u8(input)?;

        let interface_string = UsbString::new(interface_string_id).read(ctx);

        let mut i = 0;
        let mut input = input;
        // let mut endpoints: Vec<_> = vec![];
        loop {
            let kind = input[1];

            if kind == 0x21 {
                let (input_, desc) = HIDDescriptor::parse(ctx, input)?;
                input = input_;
            } else {
                let (input_, desc) = EndpointDescriptor::parse(ctx, input)?;
                input = input_;

                i += 1;
            }

            if i >= num_endpoints {
                break;
            }
        }

        let desc = InterfaceDescriptor {
            id,
            alt_setting,
            num_endpoints,
            if_class,
            if_subclass,
            if_proto,
            interface: interface_string,
        };

        log(format!("{desc}"));

        Ok((input, desc))
    }
}

impl Display for InterfaceDescriptor {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(f, "Interface({}) {{ ", self.interface)?;
        write!(f, "id: {}, ", self.id)?;
        write!(f, "setting: {}, ", self.alt_setting)?;
        write!(f, "class: {}, ", self.if_class)?;
        write!(f, "subclass: {}, ", self.if_subclass)?;
        write!(f, "proto: {}, ", self.if_proto)?;
        write!(f, "{} ep(s)}}", self.num_endpoints)?;

        Ok(())
    }
}

#[derive(Debug)]
struct EndpointDescriptor {
    address: u8,
    attributes: u8,
    mps: u16,
    interval: u8,
}

impl Descriptor for EndpointDescriptor {
    fn get_type() -> u8 {
        5
    }
    fn get_length() -> Option<usize> {
        Some(7)
    }

    fn parse<'a>(_ctx: &mut ParsingContext, input: &'a [u8]) -> NomResult<&'a [u8], Self> {
        let (input, _) = le_u8(input)?; // length
        let (input, _) = le_u8(input)?; // type
        let (input, address) = le_u8(input)?; // addr
        let (input, attributes) = le_u8(input)?; // attribute
        let (input, mps) = le_u16(input)?; // mps
        let (input, interval) = le_u8(input)?; // interval

        let desc = EndpointDescriptor {
            address,
            attributes,
            mps,
            interval,
        };

        log(format!("{desc:?}"));

        Ok((input, desc))
    }
}

#[derive(Debug)]
enum HIDClassDescriptor {
    Report { size: u16 },
    Physical { size: u16 },
    Unknown { kind: u8, size: u16 },
}

impl HIDClassDescriptor {
    fn parse<'a>(_ctx: &mut ParsingContext, input: &'a [u8]) -> NomResult<&'a [u8], Self> {
        let (input, kind) = le_u8(input)?;
        let (input, size) = le_u16(input)?;

        let desc = if kind == 0x22 {
            HIDClassDescriptor::Report { size }
        } else if kind == 0x23 {
            HIDClassDescriptor::Physical { size }
        } else {
            HIDClassDescriptor::Unknown { kind, size }
        };

        log(format!("{desc:?}"));

        Ok((input, desc))
    }
}

#[derive(Debug)]
struct HIDDescriptor {
    bcd_hid: u16,
    country_code: u8,
    class_descriptors: Vec<HIDClassDescriptor>,
}

impl Descriptor for HIDDescriptor {
    fn get_type() -> u8 {
        0x21
    }

    fn get_length() -> Option<usize> {
        Some(9)
    }

    fn parse<'a>(ctx: &mut ParsingContext, input: &'a [u8]) -> NomResult<&'a [u8], Self> {
        let (input, _) = le_u8(input)?; // length
        let (input, _) = le_u8(input)?; // type
        let (input, bcd_hid) = le_u16(input)?;
        let (input, country_code) = le_u8(input)?;
        let (input, num_descriptors) = le_u8(input)?;
        let (input, class_descriptors) = count(
            |x| HIDClassDescriptor::parse(ctx, x),
            num_descriptors.into(),
        )(input)?;

        let desc = HIDDescriptor {
            bcd_hid,
            country_code,
            class_descriptors,
        };

        log(format!("{desc:?}"));

        Ok((input, desc))
    }
}

#[derive(Debug)]
struct ConfigurationDescriptor {
    interfaces: Vec<InterfaceDescriptor>,
    id_configuration: u8,
    i_configuration: u8,
    attributes: u8,
    max_power: u8,
}

impl Descriptor for ConfigurationDescriptor {
    fn get_type() -> u8 {
        0x02
    }
    fn get_length() -> Option<usize> {
        Some(9)
    }

    fn parse<'a>(ctx: &mut ParsingContext, input: &'a [u8]) -> NomResult<&'a [u8], Self> {
        let (input, _length) = le_u8(input)?; // length
        let (input, _type) = le_u8(input)?; // type
        let (input, _total_length) = le_u16(input)?;
        let (input, num_interface) = le_u8(input)?;
        let (input, configuration_value) = le_u8(input)?;
        let (input, i_configuration) = le_u8(input)?;
        let (input, attributes) = le_u8(input)?;
        let (input, max_power) = le_u8(input)?;

        let (input, interfaces) =
            count(|x| InterfaceDescriptor::parse(ctx, x), num_interface.into())(input)?;

        let desc = ConfigurationDescriptor {
            interfaces,
            id_configuration: configuration_value,
            i_configuration,
            attributes,
            max_power: max_power.try_into().unwrap(),
        };

        log(format!("{desc:?}"));

        Ok((input, desc))
    }

    fn new(ctx: &mut ParsingContext, index: u8) -> Option<Self> {
        let total_length: u16 = {
            let buf = &mut [0; 4];
            ctx.ep0.get_descriptor(1, index, buf, 4);

            (buf[4] as u16) << 8 | buf[3] as u16
        };

        // usually object construction
        let mut buf = vec![0; total_length.into()];
        ctx.ep0
            .get_descriptor(1, index, buf.as_mut_slice(), total_length.into());
        Self::parse(ctx, buf.as_mut_slice()).map(|x| x.1).ok()
    }
}

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

    let mut ep0 = Box::new(EP0::new(0));

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

        let config = ConfigurationDescriptor::parse(&mut ctx, buf.as_mut_slice());
    }
}

#[no_mangle]
pub extern "C" fn usb_rs_run() {
    run();
}
