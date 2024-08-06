use alloc::borrow::Cow;

#[cfg(target_os = "none")]
use alloc::ffi::CString;

#[cfg(target_os = "none")]
use crate::binding_basic;

#[cfg(target_os = "none")]
#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}

#[cfg(target_os = "none")]
pub fn sleep_ms(ms: i32) {
    unsafe {
        binding_basic::sleep_ms(ms);
    }
}

#[cfg(not(target_os = "none"))]
pub fn sleep_ms(ms: i32) {
    std::thread::sleep(std::time::Duration::from_millis(ms as u64));
}

#[cfg(target_os = "none")]
pub fn log<'a, S: Into<Cow<'a, str>>>(message: S) {
    let s: Cow<'a, str> = message.into();
    let s: &str = &s;
    let s = CString::new(s).unwrap();
    let s = s.as_c_str();

    unsafe {
        binding_basic::__syoch_put_log(s.as_ptr());
    }
}

#[cfg(not(target_os = "none"))]
pub fn log<'a, S: Into<Cow<'a, str>>>(message: S) {
    let s: Cow<'a, str> = message.into();
    let s: &str = &s;

    println!("{}", s);
}