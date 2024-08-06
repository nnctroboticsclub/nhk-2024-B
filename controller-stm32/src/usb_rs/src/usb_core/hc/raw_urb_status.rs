use crate::binding::{
    stm32_usb_host_UrbStatus, stm32_usb_host_UrbStatus_kDone, stm32_usb_host_UrbStatus_kError,
    stm32_usb_host_UrbStatus_kIdle, stm32_usb_host_UrbStatus_kNYet,
    stm32_usb_host_UrbStatus_kNotReady, stm32_usb_host_UrbStatus_kStall,
};

use super::urb_status::URBStatus;

impl From<stm32_usb_host_UrbStatus> for URBStatus {
    fn from(status: stm32_usb_host_UrbStatus) -> Self {
        match status {
            stm32_usb_host_UrbStatus_kIdle => URBStatus::Idle,
            stm32_usb_host_UrbStatus_kDone => URBStatus::Done,
            stm32_usb_host_UrbStatus_kNotReady => URBStatus::NotReady,
            stm32_usb_host_UrbStatus_kNYet => URBStatus::NYet,
            stm32_usb_host_UrbStatus_kError => URBStatus::Error,
            stm32_usb_host_UrbStatus_kStall => URBStatus::Stall,
            _ => URBStatus::Unknown,
        }
    }
}
