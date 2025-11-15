add_executable(robot3 main.cpp)

find_package(ikarashiCAN_mk2 REQUIRED)
find_package(PS4_RX REQUIRED)
find_package(Futaba_Puropo REQUIRED)

target_link_libraries(robot3 PUBLIC
  StaticMbedOS
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

static_mbed_os_app_target(robot3)