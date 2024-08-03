use core::fmt::{self, Debug, Formatter};

pub enum TransactionToken {
    Setup,
    In,
    Out,
}

impl Debug for TransactionToken {
    fn fmt(&self, f: &mut Formatter) -> fmt::Result {
        match self {
            TransactionToken::Setup => write!(f, "Setup"),
            TransactionToken::In => write!(f, "   In"),
            TransactionToken::Out => write!(f, "  Out"),
        }
    }
}

impl TransactionToken {
    pub fn is_outgoing_token(&self) -> bool {
        match self {
            TransactionToken::Out => true,
            TransactionToken::Setup => true,

            _ => false,
        }
    }
}
