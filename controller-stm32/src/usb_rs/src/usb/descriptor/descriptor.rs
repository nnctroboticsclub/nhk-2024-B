use crate::usb::{ep0::EP0, ParsingContext};
use core::option::Option;
use nom::IResult;

pub trait Descriptor: Sized {
    fn get_type() -> u8;
    fn get_length() -> Option<usize>;

    fn parse<'a>(ctx: &mut ParsingContext<impl EP0>, buf: &'a [u8]) -> IResult<&'a [u8], Self>;
}
