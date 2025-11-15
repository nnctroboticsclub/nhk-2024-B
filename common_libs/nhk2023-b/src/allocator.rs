extern crate alloc;

use crate::mem;
use core::alloc::GlobalAlloc;

struct FFIAllocator;

unsafe impl GlobalAlloc for FFIAllocator {
    unsafe fn alloc(&self, layout: core::alloc::Layout) -> *mut u8 {
        let size = layout.size().try_into().unwrap();
        let ptr = mem::__ffi_malloc(size) as *mut u8;

        ptr
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _layout: core::alloc::Layout) {
        mem::__ffi_free(ptr as *mut core::ffi::c_void);
    }
}

#[global_allocator]
static ALLOCATOR: FFIAllocator = FFIAllocator;
