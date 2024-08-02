use alloc::{borrow::Cow, ffi::CString};

use crate::binding_basic;

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}

pub fn sleep_ms(ms: i32) {
    unsafe {
        binding_basic::sleep_ms(ms);
    }
}

pub fn log<'a, S: Into<Cow<'a, str>>>(message: S) {
    let s: Cow<'a, str> = message.into();
    let s: &str = &s;
    let s = CString::new(s).unwrap();
    let s = s.as_c_str();

    unsafe {
        binding_basic::__syoch_put_log(s.as_ptr());
    }

    sleep_ms(100);
}
