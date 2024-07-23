#[derive(Debug)]
pub enum URBResult {
    Success,
    Error(URBError),
    TransactError,
}

impl From<URBError> for URBResult {
    fn from(error: URBError) -> Self {
        URBResult::Error(error)
    }
}
