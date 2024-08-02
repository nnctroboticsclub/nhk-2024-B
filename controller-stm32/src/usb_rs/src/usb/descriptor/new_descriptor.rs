use alloc::vec;

use crate::usb::{ep0::EP0, ParsingContext};

use super::descriptor::Descriptor;

pub trait NewDescriptor<T: EP0>: Descriptor {
    fn new(ctx: &mut ParsingContext<T>, index: u8) -> Option<Self> {
        let length: u16 = Self::get_length()?.try_into().ok()?;

        let mut buf = vec![0; length as usize];

        ctx.ep0
            .get_descriptor(Self::get_type(), index, buf.as_mut_slice(), length);

        Self::parse(ctx, buf.as_mut_slice()).map(|x| x.1).ok()
    }
}
