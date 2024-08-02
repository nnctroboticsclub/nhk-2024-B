use core::alloc::GlobalAlloc;

use crate::binding_basic::{free, malloc};

struct FFIAllocator;

unsafe impl GlobalAlloc for FFIAllocator {
    unsafe fn alloc(&self, layout: core::alloc::Layout) -> *mut u8 {
        malloc(layout.size().try_into().unwrap()) as *mut u8
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _layout: core::alloc::Layout) {
        free(ptr as *mut core::ffi::c_void);
    }
}

#[global_allocator]
static ALLOCATOR: FFIAllocator = FFIAllocator;
