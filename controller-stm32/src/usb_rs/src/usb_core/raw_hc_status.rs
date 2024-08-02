use crate::binding::{
    stm32_usb_host_HCStatus, stm32_usb_host_HCStatus_kBabbleErr,
    stm32_usb_host_HCStatus_kDataToggleErr, stm32_usb_host_HCStatus_kHalted,
    stm32_usb_host_HCStatus_kIdle, stm32_usb_host_HCStatus_kNYet, stm32_usb_host_HCStatus_kNak,
    stm32_usb_host_HCStatus_kStall, stm32_usb_host_HCStatus_kXActErr,
    stm32_usb_host_HCStatus_kXfrc,
};

#[derive(Debug)]
pub enum RawHCStatus {
    Idle,
    Xfrc,
    Halted,
    Nak,
    NYet,
    Stall,
    XActErr,
    BabbleErr,
    DataToggleErr,

    Unknown,
}

impl From<stm32_usb_host_HCStatus> for RawHCStatus {
    fn from(status: stm32_usb_host_HCStatus) -> Self {
        use RawHCStatus::*;

        match status {
            stm32_usb_host_HCStatus_kIdle => Idle,
            stm32_usb_host_HCStatus_kXfrc => Xfrc,
            stm32_usb_host_HCStatus_kHalted => Halted,
            stm32_usb_host_HCStatus_kNak => Nak,
            stm32_usb_host_HCStatus_kNYet => NYet,
            stm32_usb_host_HCStatus_kStall => Stall,
            stm32_usb_host_HCStatus_kXActErr => XActErr,
            stm32_usb_host_HCStatus_kBabbleErr => BabbleErr,
            stm32_usb_host_HCStatus_kDataToggleErr => DataToggleErr,
            _ => Unknown,
        }
    }
}
