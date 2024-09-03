add_library(usb_otg_packets OBJECT)
target_include_directories(usb_otg_packets PUBLIC
  .
)
target_sources(usb_otg_packets PRIVATE
  usb/hcd.cpp
  usb/hc.cpp
)
target_link_libraries(usb_otg_packets PRIVATE
  mbed-os syoch-robotics
)