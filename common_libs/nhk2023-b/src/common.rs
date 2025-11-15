extern crate alloc;

extern "C" {
    fn puts(data: *const u8);
}

#[cfg(target_arch = "arm")]
#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    unsafe {
        puts("panic\0".as_ptr());
    }

    loop {}
}
