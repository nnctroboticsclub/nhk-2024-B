use alloc::format;

use crate::{
    common::log,
    usb::{ParsingContext, EP0},
};
use nom::{
    number::complete::{le_u16, le_u8},
    IResult,
};

#[derive(Debug)]
pub enum HIDClassDescriptor {
    Report { size: u16 },
    Physical { size: u16 },
    Unknown { kind: u8, size: u16 },
}

impl HIDClassDescriptor {
    pub fn parse<'a>(
        _ctx: &mut ParsingContext<impl EP0>,
        input: &'a [u8],
    ) -> IResult<&'a [u8], Self> {
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
