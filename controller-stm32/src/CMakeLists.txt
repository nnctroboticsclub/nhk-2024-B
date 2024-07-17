#
# usb_otg_packets
#
add_library(usb_otg_packets STATIC)
target_include_directories(usb_otg_packets PUBLIC
  usb_otg_packets
)
target_sources(usb_otg_packets PRIVATE
  usb_otg_packets/usb/hcd.cpp
  usb_otg_packets/usb/hc.cpp
)
target_link_libraries(usb_otg_packets PRIVATE
  mbed-os syoch-robotics
)

#
# usb_otg_cpp
#
add_library(usb_otg_cpp STATIC)
target_include_directories(usb_otg_cpp PUBLIC
  usb_otg_cpp
)
target_sources(usb_otg_cpp PRIVATE
  usb_otg_cpp/usb/endpoint.cpp
  usb_otg_cpp/usb/device.cpp
)
target_link_libraries(usb_otg_cpp PRIVATE
  mbed-os syoch-robotics usb_otg_packets
)

#
# ControllerSTM32
#
add_executable(ControllerSTM32)
target_sources(ControllerSTM32 PRIVATE
  main.cpp
)


target_link_libraries(ControllerSTM32 PRIVATE
  mbed-os syoch-robotics usb_otg_cpp usb_otg_packets
)
mbed_set_post_build(ControllerSTM32)