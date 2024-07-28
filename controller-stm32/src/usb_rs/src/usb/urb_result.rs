use super::RawURBStatus;

#[derive(Debug)]
pub enum URBError {
    NYet,
    Error,
    Stall,

    Unknown,
}

#[derive(Debug)]
pub enum URBResult {
    Idle,
    Busy,
    Done,
    Error(URBError),
}

impl From<URBError> for URBResult {
    fn from(error: URBError) -> Self {
        URBResult::Error(error)
    }
}

impl From<RawURBStatus> for URBResult {
    fn from(value: RawURBStatus) -> Self {
        match value {
            RawURBStatus::Idle => URBResult::Idle,
            RawURBStatus::Done => URBResult::Done,
            RawURBStatus::NotReady => URBResult::Busy,
            RawURBStatus::NYet => URBError::NYet.into(),
            RawURBStatus::Error => URBError::Error.into(),
            RawURBStatus::Stall => URBError::Stall.into(),
            RawURBStatus::Unknown => URBResult::Error(URBError::Unknown),
        }
    }
}
