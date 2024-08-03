use crate::usb_core::hc::TransactionResult;

pub trait PhysicalEP0 {
    fn set_max_packet_size(&mut self, mps: u8);
    fn set_address(&mut self, new_address: u8) -> TransactionResult<()>;
}
