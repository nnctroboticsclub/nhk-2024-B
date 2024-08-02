use super::URBResult;

#[derive(Debug)]
pub enum HCError {
    TransactError,
    Babble,
    DataToggle,
}

#[derive(Debug)]
pub enum HCStatus {
    Idle,
    Busy,
    Done(URBResult),
    Error(HCError),
}
