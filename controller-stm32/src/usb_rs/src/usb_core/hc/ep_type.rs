#[derive(Debug, Copy, Clone, PartialEq)]
pub enum EPType {
    Control,
    Isochronous,
    Bulk,
    Interrupt,
}
