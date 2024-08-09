use core::fmt::Display;

#[derive(Debug, PartialEq, Eq, PartialOrd, Ord)]
pub enum TransactionError {
    Timeout,
    NotReady,
    Error, // Generic error
}

impl Display for TransactionError {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        match self {
            TransactionError::Timeout => write!(f, "Timeout"),
            TransactionError::NotReady => write!(f, "NotReady"),
            TransactionError::Error => write!(f, "Error"),
        }
    }
}

impl core::error::Error for TransactionError {}

pub type TransactionResult<T> = Result<T, TransactionError>;
