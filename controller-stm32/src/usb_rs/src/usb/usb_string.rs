use alloc::string::{String, ToString};

use crate::usb_core::hc::TransactionResult;

use super::{ParsingContext, EP0};

pub struct UsbString(u8);

impl UsbString {
    pub fn new(id: u8) -> UsbString {
        UsbString(id)
    }

    pub fn read(&self, ctx: &mut ParsingContext<impl EP0>) -> TransactionResult<String> {
        if self.0 != 0 {
            ctx.ep0.get_string(self.0)
        } else {
            Ok("-----".to_string())
        }
    }
}
