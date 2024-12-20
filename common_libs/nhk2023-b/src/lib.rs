#![cfg_attr(not(test), no_std)]

pub use im920_rs::ffi::{
    __ffi_cim920_get_node_number, __ffi_cim920_get_version, __ffi_cim920_new, __ffi_cim920_on_data,
    __ffi_cim920_transmit_delegate, CIM920,
};

pub use srobo_base::{
    communication::{CStreamRx, CStreamTx, __ffi_cstream_associate_tx, __ffi_cstream_feed_rx},
    time::{CTime, __ffi_ctime_set_now, __ffi_ctime_set_sleep},
};

mod allocator;
mod common;
mod ffi_mem;
mod logger;
