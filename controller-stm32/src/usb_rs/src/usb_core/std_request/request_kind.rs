#[derive(Debug, Clone, Copy)]
pub enum RequestKind {
    Standard,
    Class,
    Vendor,
    Reserved,
}
