#![cfg_attr(target_os = "none", no_std)]
#![feature(str_from_utf16_endian)]

#[cfg(target_os = "none")]
mod binding;

#[cfg(target_os = "none")]
mod binding_basic;

#[cfg(target_os = "none")]
mod allocator;

mod common;
mod logger;
mod usb;
mod usb_core;

#[cfg(target_os = "none")]
mod app;

extern crate alloc;

#[cfg(not(target_os = "none"))]
pub fn main() {
    println!("Hello, world!");
}
