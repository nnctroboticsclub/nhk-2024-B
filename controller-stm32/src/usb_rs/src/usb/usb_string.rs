use alloc::{
    format,
    string::{String, ToString},
};

use crate::common::log;

use super::{ParsingContext, EP0};

pub struct UsbString(u8);

impl UsbString {
    pub fn new(id: u8) -> UsbString {
        UsbString(id)
    }

    pub fn read(&self, ctx: &mut ParsingContext<impl EP0>) -> String {
        if self.0 != 0 {
            log(format!("Using Normal getstring {}", self.0));
            ctx.ep0.get_string(self.0)
        } else {
            log(format!("fallback ({})", self.0));
            "-----".to_string()
        }
    }
}
