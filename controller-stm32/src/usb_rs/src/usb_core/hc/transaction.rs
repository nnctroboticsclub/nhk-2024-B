use core::fmt::{self, Debug};

use super::TransactionToken;

pub struct Transaction<'a> {
    pub token: TransactionToken,
    pub toggle: u8,
    pub buffer: &'a mut [u8],
    pub length: u8,
}

impl<'a> Debug for Transaction<'a> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}<{}>{{{:?};", self.token, self.toggle, self.length,)?;

        for i in 0..self.length {
            write!(f, " {:02X}", self.buffer[i as usize])?;
        }

        write!(f, "}}")
    }
}
