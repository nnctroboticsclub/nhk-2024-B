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

    CP210xIfcEnable,
    CP210xSetBaudDiv,
    CP210xSetBaudRate,
    CP210xSetLineCtrl,
    CP210xSetChars,
    CP210xGetFlow,
    CP210xSetFlow,
    CP210xSetMHS,
    CP210xGetCommStatus,
    CP210xPurge,
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

            CP210xIfcEnable => 0x00,
            CP210xSetBaudDiv => 0x01,
            CP210xSetBaudRate => 0x1E,
            CP210xSetLineCtrl => 0x03,
            CP210xSetChars => 0x19,
            CP210xGetFlow => 0x14,
            CP210xSetFlow => 0x13,
            CP210xSetMHS => 0x07,
            CP210xGetCommStatus => 0x10,
            CP210xPurge => 0x12,
        }
    }
}
