use super::URBResult;

#[derive(Debug)]
pub enum HCStatus {
    Busy,
    Done(URBResult),
    Unknown,
}

impl From<URBResult> for HCStatus {
    fn from(result: URBResult) -> Self {
        HCStatus::Done(result)
    }
}
