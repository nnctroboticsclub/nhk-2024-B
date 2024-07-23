use std::env;
use std::path::PathBuf;

fn main() {
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());

    bindgen::Builder::default()
        .header("../usb_otg_packets/usb/packets_wrapper.hpp")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks {}))
        .use_core()
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");

    bindgen::Builder::default()
        .header("../../syoch-robotics/robotics/logger/logger.hpp")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks {}))
        .use_core()
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file(out_path.join("bindings-logger.rs"))
        .expect("Couldn't write bindings!");

    bindgen::Builder::default()
        .header("../wrapper/inc/wrapper.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks {}))
        .use_core()
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file(out_path.join("bindings-basic.rs"))
        .expect("Couldn't write bindings!");
}
