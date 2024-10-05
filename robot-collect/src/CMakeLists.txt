add_executable(robot3)
target_sources(robot3 PRIVATE
  main.cpp
)

find_package(ikarashiCAN_mk2 REQUIRED)
find_package(PS4_RX REQUIRED)
find_package(Futaba_Puropo REQUIRED)

target_link_libraries(robot3 PRIVATE
  mbed-os
  syoch-robotics-logger
  syoch-robotics-mbed-uart
  syoch-robotics-gpio
  fep
  bd621x
  ikarashiCAN_mk2
  PS4_RX
  NHK2024B
  Futaba_Puropo
)

mbed_set_post_build(robot3)