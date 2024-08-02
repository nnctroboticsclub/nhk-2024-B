use alloc::string::String;

pub trait EP0 {
    fn get_descriptor(&mut self, descriptor_type: u8, index: u8, buf: &mut [u8], length: u16);
    fn get_string(&mut self, index: u8) -> String;
}
