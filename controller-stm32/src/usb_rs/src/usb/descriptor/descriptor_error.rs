use core::fmt::Display;

use crate::usb_core::hc::TransactionError;

#[derive(Debug)]
pub enum DescriptorError {
    TransactionError(TransactionError),
    GeneralParseError,
    InvalidLength,
    InvalidType,
}

impl Display for DescriptorError {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        match self {
            DescriptorError::TransactionError(e) => write!(f, "Transaction error: {}", e),
            DescriptorError::GeneralParseError => write!(f, "General parse error"),
            DescriptorError::InvalidLength => write!(f, "Invalid length"),
            DescriptorError::InvalidType => write!(f, "Invalid type"),
        }
    }
}

impl core::error::Error for DescriptorError {}

pub type DescriptorResult<T> = Result<T, DescriptorError>;

impl From<TransactionError> for DescriptorError {
    fn from(e: TransactionError) -> Self {
        DescriptorError::TransactionError(e)
    }
}
