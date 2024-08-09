// TODO: Refactor this

#[derive(Debug, Clone, Copy)]
pub enum RequestByte {
    GetStatus,
    ClearFeature,
    SetFeature,
    SetAddress,
    GetDescriptor,
    SetDescriptor,
    GetConfiguration,
    SetConfiguration,
    GetInterface,
    SetInterface,
    SynchFrame,

    HidGetReport,
    HidSetReport,
}

impl Into<u8> for RequestByte {
    fn into(self) -> u8 {
        use RequestByte::*;
        match self {
            GetStatus => 0u8,
            ClearFeature => 1u8,
            SetFeature => 3u8,
            SetAddress => 5u8,
            GetDescriptor => 6u8,
            SetDescriptor => 7u8,
            GetConfiguration => 8u8,
            SetConfiguration => 9u8,
            GetInterface => 10u8,
            SetInterface => 11u8,
            SynchFrame => 12u8,

            HidGetReport => 1u8,
            HidSetReport => 9u8,
        }
    }
}
