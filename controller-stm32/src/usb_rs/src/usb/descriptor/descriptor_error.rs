use crate::usb_core::hc::TransactionError;

#[derive(Debug)]
pub enum DescriptorError {
    TransactionError(TransactionError),
    GeneralParseError,
    InvalidLength,
    InvalidType,
}

pub type DescriptorResult<T> = Result<T, DescriptorError>;

impl From<TransactionError> for DescriptorError {
    fn from(e: TransactionError) -> Self {
        DescriptorError::TransactionError(e)
    }
}
