use core::fmt::Display;

#[derive(Debug)]
pub enum TransactionError {
    Timeout,
}

impl Display for TransactionError {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        match self {
            TransactionError::Timeout => write!(f, "Timeout"),
        }
    }
}

impl core::error::Error for TransactionError {}

pub type TransactionResult<T> = Result<T, TransactionError>;
