use super::{direction::Direction, recipient::Recipient, request_kind::RequestKind};

#[derive(Debug)]
pub struct RequestType {
    pub direction: Direction,
    pub req_type: RequestKind,
    pub recipient: Recipient,
}

impl Into<u8> for RequestType {
    fn into(self) -> u8 {
        let direction = match self.direction {
            Direction::HostToDevice => 0b0_00_00000,
            Direction::DeviceToHost => 0b1_00_00000,
        };

        let req_type = match self.req_type {
            RequestKind::Standard => 0b0_00_00000,
            RequestKind::Class => 0b0_01_00000,
            RequestKind::Vendor => 0b0_10_00000,
            RequestKind::Reserved => 0b0_11_00000,
        };

        let recipient = match self.recipient {
            Recipient::Device => 0b0_00_00000,
            Recipient::Interface => 0b0_00_00001,
            Recipient::Endpoint => 0b0_00_00010,
            Recipient::Other => 0b0_00_00011,
        };

        direction | req_type | recipient
    }
}
