use core::fmt::{self, Debug};

#[derive(Clone, Copy)]
pub struct TransactionDestination {
    pub dev: u8,
    pub ep: u8,
}

impl Debug for TransactionDestination {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{dev:02X}-{ep:02X}", dev = self.dev, ep = self.ep)
    }
}
