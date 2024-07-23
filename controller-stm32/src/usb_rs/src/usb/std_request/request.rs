use super::{request_byte::RequestByte, request_type::RequestType};

#[derive(Debug)]
pub struct StdRequest {
    pub request_type: RequestType,
    pub request: RequestByte,
    pub value: u16,
    pub index: u16,
    pub length: u16,
}

impl Into<[u8; 8]> for StdRequest {
    fn into(self) -> [u8; 8] {
        let mut buffer: [u8; 8] = [0; 8];

        buffer[0] = self.request_type.into();
        buffer[1] = self.request.into();
        buffer[2] = (self.value & 0xFF) as u8;
        buffer[3] = (self.value >> 8) as u8;
        buffer[4] = (self.index & 0xFF) as u8;
        buffer[5] = (self.index >> 8) as u8;
        buffer[6] = (self.length & 0xFF) as u8;
        buffer[7] = (self.length >> 8) as u8;

        buffer
    }
}
