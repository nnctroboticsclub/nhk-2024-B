add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/libusb_rs.a"
  COMMAND echo hi
  COMMAND /home/vscode/.cargo/bin/cargo build
  COMMAND cp target/thumbv7em-none-eabi/debug/libusb_rs.a "${CMAKE_BINARY_DIR}/libusb_rs.a"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_target(usb_rs_target DEPENDS "${CMAKE_BINARY_DIR}/libusb_rs.a")

add_library(usb_rs STATIC IMPORTED GLOBAL)
add_dependencies(usb_rs usb_rs_target)
set_target_properties(usb_rs
    PROPERTIES
    IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/libusb_rs.a"
)