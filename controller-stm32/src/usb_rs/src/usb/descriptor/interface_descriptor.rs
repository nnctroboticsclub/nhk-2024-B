use core::fmt::Display;

use alloc::{format, string::String};

use nom::{number::complete::le_u8, IResult};

use crate::{
    common::log,
    usb::{ParsingContext, UsbString, EP0},
};

use super::{descriptor::Descriptor, hid_descriptor::HIDDescriptor, EndpointDescriptor};

#[derive(Debug)]
pub struct InterfaceDescriptor {
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

    fn parse<'a>(ctx: &mut ParsingContext<impl EP0>, input: &'a [u8]) -> IResult<&'a [u8], Self> {
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
                let (input_, _desc) = HIDDescriptor::parse(ctx, input)?;
                input = input_;
            } else {
                let (input_, _desc) = EndpointDescriptor::parse(ctx, input)?;
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
