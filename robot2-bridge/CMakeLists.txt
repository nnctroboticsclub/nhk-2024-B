find_package(PS4_RX REQUIRED)
find_package(ikarashiCAN_mk2 REQUIRED)
find_package(can_servo REQUIRED)
find_package(ikako_rohm_md REQUIRED)
find_package(StaticMbedOS REQUIRED)

add_executable(robot2 main.cpp)

target_link_libraries(robot2 PRIVATE
  StaticMbedOS
  NHK2024B
  PS4_RX
  ikarashiCAN_mk2
  fep
  syoch-robotics-simple-can
  syoch-robotics-mbed-uart
  syoch-robotics-ikako-robomas
  ikako_rohm_md
  can_servo
  Futaba_Puropo
)

static_mbed_os_app_target(robot2)