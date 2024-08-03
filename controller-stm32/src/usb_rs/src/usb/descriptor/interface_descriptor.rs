use core::fmt::Debug;

use alloc::{format, string::String};

use nom::{number::complete::le_u8, IResult};

use crate::usb::{ParsingContext, UsbString, EP0};

use super::{descriptor::Descriptor, hid_descriptor::HIDDescriptor, EndpointDescriptor};

pub struct InterfaceDescriptor {
    pub id: u8,
    pub name: String,

    pub if_class: u8,
    pub if_subclass: u8,
    pub if_proto: u8,

    pub alt_setting: u8,
    pub num_endpoints: u8,
}

impl Descriptor for InterfaceDescriptor {
    fn get_type() -> u8 {
        4
    }
    fn get_length() -> Option<usize> {
        Some(9)
    }

    fn parse<'a>(ctx: &mut ParsingContext<impl EP0>, input: &'a [u8]) -> IResult<&'a [u8], Self> {
        let (input, _) = le_u8(input)?; // length
        let (input, _) = le_u8(input)?; // type
        let (input, id) = le_u8(input)?;
        let (input, alt_setting) = le_u8(input)?;
        let (input, num_endpoints) = le_u8(input)?;
        let (input, if_class) = le_u8(input)?;
        let (input, if_subclass) = le_u8(input)?;
        let (input, if_proto) = le_u8(input)?;
        let (input, interface_string_id) = le_u8(input)?;

        let interface_string = match UsbString::new(interface_string_id).read(ctx) {
            Ok(s) => s,
            Err(e) => format!("Error[{:?}]", e),
        };

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
            name: interface_string,
        };

        Ok((input, desc))
    }
}

impl Debug for InterfaceDescriptor {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(f, "Interface{}({}) {{ ", self.id, self.name)?;
        write!(f, "setting: {}, ", self.alt_setting)?;
        write!(f, "class: {}, ", self.if_class)?;
        write!(f, "subclass: {}, ", self.if_subclass)?;
        write!(f, "proto: {}, ", self.if_proto)?;
        write!(f, "{} ep(s)", self.num_endpoints)?;
        write!(f, "}}")?;

        Ok(())
    }
}
