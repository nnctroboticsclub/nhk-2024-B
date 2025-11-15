extern "C" {
    pub fn __ffi_malloc(size: usize) -> *mut core::ffi::c_void;
    pub fn __ffi_free(ptr: *mut core::ffi::c_void);
}
