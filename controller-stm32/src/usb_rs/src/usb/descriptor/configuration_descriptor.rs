use super::{
    descriptor::Descriptor, interface_descriptor::InterfaceDescriptor,
    new_descriptor::NewDescriptor,
};
use crate::usb::{ParsingContext, PhysicalEP0, UsbString, EP0};

use core::fmt::Debug;
use core::option::Option::{self, Some};
use core::result::Result::Ok;
use core::write;

use alloc::{string::String, vec, vec::Vec};
use nom::{
    multi::count,
    number::complete::{le_u16, le_u8},
    IResult,
};

pub struct ConfigurationDescriptor {
    pub id: u8,
    pub name: String,
    pub attributes: u8,
    pub max_power: u8,
    pub interfaces: Vec<InterfaceDescriptor>,
}

impl Descriptor for ConfigurationDescriptor {
    fn get_type() -> u8 {
        0x02
    }
    fn get_length() -> Option<usize> {
        Some(9)
    }

    fn parse<'a>(ctx: &mut ParsingContext<impl EP0>, input: &'a [u8]) -> IResult<&'a [u8], Self> {
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

        let name = UsbString::new(i_configuration).read(ctx);

        let desc = ConfigurationDescriptor {
            interfaces,
            id: configuration_value,
            name,
            attributes,
            max_power: max_power.try_into().unwrap(),
        };

        Ok((input, desc))
    }
}

impl<T: EP0 + PhysicalEP0> NewDescriptor<T> for ConfigurationDescriptor {
    fn new(ctx: &mut ParsingContext<T>, index: u8) -> Option<Self> {
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

impl Debug for ConfigurationDescriptor {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(f, "Configuration({}) {{", self.name)?;
        write!(f, "id: {}, ", self.id)?;
        write!(f, "attributes: {}, ", self.attributes)?;
        write!(f, "max_power: {}, ", self.max_power)?;
        write!(f, "interfaces: {:?}", self.interfaces)?;
        write!(f, "}}")?;

        Ok(())
    }
}
