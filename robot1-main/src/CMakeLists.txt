find_package(PS4_RX REQUIRED)
find_package(ikarashiCAN_mk2 REQUIRED)

add_executable(robot1)
target_sources(robot1 PRIVATE
  main.cpp
)

target_link_libraries(robot1 PRIVATE
  StaticMbedOS
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

static_mbed_os_app_target(robot1)