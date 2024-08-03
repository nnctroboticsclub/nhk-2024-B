#[derive(Debug)]
pub enum TransactionError {
    Timeout,
}

pub type TransactionResult<T> = Result<T, TransactionError>;
