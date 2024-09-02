use std::env;
use std::path::PathBuf;

fn main() {
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());

    println!("cargo:rerun-if-changed=build.rs");

    bindgen::Builder::default()
        .header("../../syoch-robotics/robotics/logger/logger.hpp")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks {}))
        .use_core()
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file(out_path.join("bindings-logger.rs"))
        .expect("Couldn't write bindings!");
    println!("cargo:rerun-if-changed=../../syoch-robotics/robotics/logger/logger.hpp");

    bindgen::Builder::default()
        .header("../wrapper/inc/wrapper.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks {}))
        .use_core()
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file(out_path.join("bindings-basic.rs"))
        .expect("Couldn't write bindings!");
    println!("cargo:rerun-if-changed=../wrapper/inc/wrapper.h");
}
