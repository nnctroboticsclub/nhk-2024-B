use core::fmt::{self, Debug};

#[derive(Clone, Copy)]
pub struct TransactionDestination {
    pub dev: u8,
    pub ep: u8,
}

impl TransactionDestination {
    pub fn to_in_ep(&self) -> TransactionDestination {
        TransactionDestination {
            dev: self.dev,
            ep: self.ep | 0x80,
        }
    }

    pub fn to_out_ep(&self) -> TransactionDestination {
        TransactionDestination {
            dev: self.dev,
            ep: self.ep & 0x7F,
        }
    }
}

impl Debug for TransactionDestination {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{dev:02X}-{ep:02X}", dev = self.dev, ep = self.ep)
    }
}
