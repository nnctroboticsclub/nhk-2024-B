#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

core::include!(core::concat!(core::env!("OUT_DIR"), "/ffi_mem.rs"));
