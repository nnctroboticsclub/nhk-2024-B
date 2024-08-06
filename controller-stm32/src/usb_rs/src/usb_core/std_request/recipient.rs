#[derive(Debug, Clone, Copy)]
pub enum Recipient {
    Device,
    Interface,
    Endpoint,
    Other,
}
