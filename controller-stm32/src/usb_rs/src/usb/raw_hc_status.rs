use crate::binding::{
    stm32_usb_host_HCStatus, stm32_usb_host_HCStatus_kBabbleErr,
    stm32_usb_host_HCStatus_kDataToggleErr, stm32_usb_host_HCStatus_kDone,
    stm32_usb_host_HCStatus_kIdle, stm32_usb_host_HCStatus_kUrbFailed,
    stm32_usb_host_HCStatus_kXActErr,
};

pub enum RawHCStatus {
    Idle,
    Done,
    UrbFailed,
    XActErr,
    BabbleErr,
    DataToggleErr,
    Unknown,
}

impl From<stm32_usb_host_HCStatus> for RawHCStatus {
    fn from(status: binding::stm32_usb_host_HCStatus) -> Self {
        use RawHCStatus::*;
        match status {
            stm32_usb_host_HCStatus_kIdle => Idle,
            stm32_usb_host_HCStatus_kDone => Done,
            stm32_usb_host_HCStatus_kUrbFailed => UrbFailed,
            stm32_usb_host_HCStatus_kXActErr => XActErr,
            stm32_usb_host_HCStatus_kBabbleErr => BabbleErr,
            stm32_usb_host_HCStatus_kDataToggleErr => DataToggleErr,
            _ => Unknown,
        }
    }
}
