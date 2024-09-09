extern crate alloc;

use alloc::borrow::Cow;

#[cfg(not(test))]
use alloc::ffi::CString;

use crate::logger::Logger;
use core::option::{Option, Option::None, Option::Some};

static mut LOGGER: Option<Logger> = None;

fn get_logger() -> &'static mut Logger {
    let logger = unsafe { &mut LOGGER };
    if logger.is_none() {
        *logger = Some(Logger::new("nhk2023-b", "NHK2023-B"));
    }

    logger.as_mut().unwrap()
}

#[cfg(not(test))]
#[panic_handler]
fn panic(info: &core::panic::PanicInfo) -> ! {
    // get_logger().error("Panic");

    loop {}
}

#[cfg(not(test))]
pub fn log<'a, S: Into<Cow<'a, str>>>(message: S) {
    let s: Cow<'a, str> = message.into();
    let s: &str = &s;

    get_logger().info(s);
}

#[cfg(test)]
pub fn log<'a, S: Into<Cow<'a, str>>>(message: S) {
    let s: Cow<'a, str> = message.into();
    let s: &str = &s;

    get_logger().info(s);
}
