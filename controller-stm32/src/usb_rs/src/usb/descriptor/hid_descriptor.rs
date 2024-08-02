use alloc::{format, vec::Vec};
use nom::{
    multi::count,
    number::complete::{le_u16, le_u8},
    IResult,
};

use crate::{
    common::log,
    usb::{ParsingContext, EP0},
};

use super::{descriptor::Descriptor, hid_class_descriptor::HIDClassDescriptor};

#[derive(Debug)]
pub struct HIDDescriptor {
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

    fn parse<'a>(ctx: &mut ParsingContext<impl EP0>, input: &'a [u8]) -> IResult<&'a [u8], Self> {
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
