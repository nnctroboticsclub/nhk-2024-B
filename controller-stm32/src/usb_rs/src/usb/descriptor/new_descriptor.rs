use alloc::vec;

use crate::usb::{ep0::EP0, ParsingContext};

use super::{Descriptor, DescriptorError, DescriptorResult};

pub trait NewDescriptor<T: EP0>: Descriptor {
    fn new(ctx: &mut ParsingContext<T>, index: u8) -> DescriptorResult<Self> {
        let length = Self::get_length();
        if length.is_none() {
            return Err(DescriptorError::InvalidLength);
        }
        let length = length.unwrap().try_into();
        if length.is_err() {
            return Err(DescriptorError::InvalidLength);
        }
        let length: usize = length.unwrap();

        let mut buf = vec![0; length];

        ctx.ep0
            .get_descriptor(Self::get_type(), index, buf.as_mut_slice())?;

        Self::parse(ctx, buf.as_mut_slice())
            .map(|x| x.1)
            .map_err(|_| DescriptorError::GeneralParseError)
    }
}
