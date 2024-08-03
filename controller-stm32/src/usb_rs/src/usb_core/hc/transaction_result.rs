pub enum TransactionError {
    Timeout,
}

pub type TransactionResult = Result<(), TransactionError>;
