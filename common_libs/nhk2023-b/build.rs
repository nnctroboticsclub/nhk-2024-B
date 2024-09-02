use std::env;

use cbindgen::cargo_toml;


fn main() {
    let out_path = std::path::PathBuf::from(env::var("OUT_DIR").unwrap());

    println!("cargo:rerun-if-changed=.");
    cargo_toml::manifest(".").unwrap().
    cbindgen::Builder::new()
        .with_crate(env::var("CARGO_MANIFEST_DIR").unwrap())
        .with_language(cbindgen::Language::Cxx)
        .with_parse_include(&["im920_rs"])
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file("inc/nhk2024b/common.hpp");

    bindgen::Builder::default()
        .header("./inc/nhk2024b/ffi_mem.hpp")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks {}))
        .use_core()
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file(out_path.join("ffi_mem.rs"))
        .expect("Couldn't write bindings!");
    println!("cargo:rerun-if-changed=./inc/nhk2024b/ffi_mem.hpp");
}
