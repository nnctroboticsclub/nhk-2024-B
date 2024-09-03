use super::{direction::Direction, recipient::Recipient, request_kind::RequestKind};

#[derive(Debug, Clone, Copy)]
pub struct RequestType {
    pub direction: Direction,
    pub req_type: RequestKind,
    pub recipient: Recipient,
}

impl Into<u8> for RequestType {
    fn into(self) -> u8 {
        let direction = match self.direction {
            Direction::HostToDevice => 0b00000000,
            Direction::DeviceToHost => 0b10000000,
        };

        let req_type = match self.req_type {
            RequestKind::Standard => 0b00000000,
            RequestKind::Class => 0b00100000,
            RequestKind::Vendor => 0b01000000,
            RequestKind::Reserved => 0b01100000,
        };

        let recipient = match self.recipient {
            Recipient::Device => 0b00000000,
            Recipient::Interface => 0b00000001,
            Recipient::Endpoint => 0b00000010,
            Recipient::Other => 0b00000011,
        };

        direction | req_type | recipient
    }
}
