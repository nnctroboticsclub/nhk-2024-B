add_library(usb_otg_cpp STATIC)
target_include_directories(usb_otg_cpp PUBLIC
  .
)
target_sources(usb_otg_cpp PRIVATE
  # usb/endpoint.cpp
  # usb/device.cpp
)
target_link_libraries(usb_otg_cpp PRIVATE
  mbed-os syoch-robotics usb_otg_packets wrapper
)