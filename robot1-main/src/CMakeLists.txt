add_executable(app)
target_sources(app PRIVATE
  main.cpp
)

target_link_libraries(app PRIVATE
  mbed-os
  static-mbed-os
  NHK2024B
  PS4_RX
  ikarashiCAN_mk2
  fep
  syoch-robotics-common
  syoch-robotics-logger
  syoch-robotics-thread
  syoch-robotics-ikako-mdc
  syoch-robotics-mbed-uart
  syoch-robotics-simple-can
  syoch-robotics-gpio
)

mbed_set_post_build(app)