use crate::binding::{
    stm32_usb_host_UrbStatus, stm32_usb_host_UrbStatus_kDone, stm32_usb_host_UrbStatus_kError,
    stm32_usb_host_UrbStatus_kIdle, stm32_usb_host_UrbStatus_kNYet,
    stm32_usb_host_UrbStatus_kNotReady, stm32_usb_host_UrbStatus_kStall,
};

#[derive(Debug, PartialEq)]
pub enum RawURBStatus {
    Idle,
    Done,
    NotReady,
    NYet,
    Error,
    Stall,
    Unknown,
}

impl From<stm32_usb_host_UrbStatus> for RawURBStatus {
    fn from(status: stm32_usb_host_UrbStatus) -> Self {
        match status {
            stm32_usb_host_UrbStatus_kIdle => RawURBStatus::Idle,
            stm32_usb_host_UrbStatus_kDone => RawURBStatus::Done,
            stm32_usb_host_UrbStatus_kNotReady => RawURBStatus::NotReady,
            stm32_usb_host_UrbStatus_kNYet => RawURBStatus::NYet,
            stm32_usb_host_UrbStatus_kError => RawURBStatus::Error,
            stm32_usb_host_UrbStatus_kStall => RawURBStatus::Stall,
            _ => RawURBStatus::Unknown,
        }
    }
}
