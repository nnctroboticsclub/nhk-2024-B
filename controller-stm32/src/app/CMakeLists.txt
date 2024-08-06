add_executable(ControllerSTM32)
target_sources(ControllerSTM32 PRIVATE
  main.cpp
)


target_link_libraries(ControllerSTM32 PRIVATE
  mbed-os syoch-robotics usb_otg_packets usb_otg_cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/usb_rs/target/thumbv7em-none-eabi/debug/libusb_rs.a
)
mbed_set_post_build(ControllerSTM32)