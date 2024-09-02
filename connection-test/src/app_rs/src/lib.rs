#![cfg_attr(target_os = "none", no_std)]
#![feature(str_from_utf16_endian)]

use core::ffi;

use alloc::{format, string::String};
use common::log;
use srobo_base::communication::{AsyncReadableStream, WritableStream};

#[cfg(target_os = "none")]
mod binding_basic;

#[cfg(target_os = "none")]
mod allocator;

mod common;

extern crate alloc;

fn run() -> Result<(), String> {
    Ok(())
}

#[no_mangle]
pub extern "C" fn app_rs_main() {
    let res = run();
    if res.is_err() {
        log(format!("Error: {:?}", res.err()));
    }

    log("Rust code finished");
}

#[cfg(not(target_os = "none"))]
pub fn main() {
    println!("Hello, world!");
}
