#
# cpp_usb_otg
#
add_library(cpp_usb_otg STATIC)
target_include_directories(cpp_usb_otg INTERFACE
  .
)
target_sources(cpp_usb_otg PRIVATE
  otg/hcd.cpp otg/hc.cpp otg/endpoint.cpp otg/device.cpp
)
target_link_libraries(cpp_usb_otg PRIVATE
  mbed-os syoch-robotics
)

#
# ControllerSTM32
#
add_executable(ControllerSTM32)
target_sources(ControllerSTM32 PRIVATE
  main.cpp
)


target_link_libraries(ControllerSTM32 PRIVATE
  mbed-os syoch-robotics cpp_usb_otg
)
mbed_set_post_build(ControllerSTM32)