#[derive(Debug, PartialEq)]
pub enum URBStatus {
    Idle,
    Done,
    NotReady,
    NYet,
    Error,
    Stall,
    Unknown,
}
