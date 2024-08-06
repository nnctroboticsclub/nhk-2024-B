add_subdirectory(wrapper)
add_subdirectory(usb_otg_cpp)
add_subdirectory(usb_otg_packets)
add_subdirectory(usb_rs)


#
# ControllerSTM32
#
add_executable(ControllerSTM32)
target_sources(ControllerSTM32 PRIVATE
  app/main.cpp
)


target_link_libraries(ControllerSTM32 PRIVATE
  mbed-os syoch-robotics usb_otg_packets usb_otg_cpp
  #${CMAKE_CURRENT_SOURCE_DIR}/usb_rs/target/thumbv7em-none-eabi/debug/libusb_rs.a
  usb_rs
)
mbed_set_post_build(ControllerSTM32)