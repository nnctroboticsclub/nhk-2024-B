#[derive(Debug)]
pub enum URBError {
    Halt,
    Nak,
    NYet,
    Stall,
    Babble,
    DataToggle,
}
