use nom::{
    number::complete::{le_u16, le_u8},
    IResult,
};

use crate::usb::{ParsingContext, EP0};

use super::descriptor::Descriptor;

#[derive(Debug)]
pub struct EndpointDescriptor {
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

    fn parse<'a>(_ctx: &mut ParsingContext<impl EP0>, input: &'a [u8]) -> IResult<&'a [u8], Self> {
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

        Ok((input, desc))
    }
}
