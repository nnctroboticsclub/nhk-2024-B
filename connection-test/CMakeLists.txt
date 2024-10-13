find_package(ikarashiCAN_mk2 REQUIRED)
find_package(MotorController REQUIRED)
find_package(IkakoRobomas REQUIRED)
find_package(can_servo REQUIRED)

add_executable(connection-test)
target_compile_features(connection-test PUBLIC cxx_std_20)

target_sources(connection-test PRIVATE
  main.cpp
)

target_link_libraries(connection-test PRIVATE
  StaticMbedOS
  syoch-robotics-common
  syoch-robotics-random
  syoch-robotics-thread
  syoch-robotics-gpio
  syoch-robotics-simple-can
  syoch-robotics-mbed-uart
  NHK2024B
  srobo_base_hxx
  im920_rs_hxx
  ikarashiCAN_mk2
  MotorController
  IkakoRobomas
  can_servo
  rd16
)

static_mbed_os_app_target(connection-test)