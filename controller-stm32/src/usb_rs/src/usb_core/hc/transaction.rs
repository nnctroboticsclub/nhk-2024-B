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
        write!(
            f,
            "{}{:?}\x1b[m<{}>{{{:3?};",
            self.token.get_color(),
            self.token,
            self.toggle,
            self.length,
        )?;

        for i in 0..self.length {
            if i % 4 == 0 {
                write!(f, " \x1b[{}m", if i & 4 == 0 { 34 } else { 36 })?;
            }
            write!(f, "{:02X}", self.buffer[i as usize])?;

            if i % 4 == 3 {
                write!(f, "\x1b[0m")?;
            }
        }

        write!(f, "\x1b[m]}}")
    }
}
