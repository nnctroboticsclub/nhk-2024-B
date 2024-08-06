use alloc::string::String;

use crate::usb_core::hc::TransactionResult;

pub trait EP0 {
    fn get_descriptor(
        &mut self,
        descriptor_type: u8,
        index: u8,
        buf: &mut [u8],
    ) -> TransactionResult<()>;
    fn get_string(&mut self, index: u8) -> TransactionResult<String>;
}
