use std::env;

fn main() {
    let out_path = std::path::PathBuf::from(env::var("OUT_DIR").unwrap());

    println!("cargo:rerun-if-changed=.");

    bindgen::Builder::default()
        .header("./inc/nhk2024b/ffi_mem.hpp")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks {}))
        .use_core()
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file(out_path.join("ffi_mem.rs"))
        .expect("Couldn't write bindings!");
    println!("cargo:rerun-if-changed=./inc/nhk2024b/ffi_mem.hpp");

    bindgen::Builder::default()
        .header("../../syoch-robotics/libs/logger/include/logger/logger.hpp")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks {}))
        .use_core()
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file(out_path.join("ffi_logger.rs"))
        .expect("Couldn't write bindings!");
    println!("cargo:rerun-if-changed=../../syoch-robotics/libs/logger/include/logger/logger.hpp");
}
