cmake_minimum_required(VERSION 3.19)
cmake_policy(VERSION 3.19)

set(MBED_APP_JSON_PATH mbed_app.json)

include(mbed-os/tools/cmake/app.cmake)
add_subdirectory(mbed-os)

add_subdirectory(syoch-robotics)

#
# MotorController
#
add_library(MotorController STATIC)
target_include_directories(MotorController PUBLIC
  ./MotorController
)
target_include_directories(MotorController PRIVATE
  ./MotorController/Ikako_PID
  ./MotorController/DisturbanceObserver
  ./MotorController/DisturbanceObserver/LowPassFilter/LowPassFilter.cpp
)
target_sources(MotorController PRIVATE
  ./MotorController/MotorController.cpp
  ./MotorController/Ikako_PID/ikako_PID.cpp
  ./MotorController/DisturbanceObserver/DOB.cpp
  ./MotorController/DisturbanceObserver/LowPassFilter/LowPassFilter.cpp
)
target_link_libraries(MotorController PRIVATE
  mbed-os ikarashiCAN_mk2
)

#
# IkakoRobomas
#
add_library(IkakoRobomas STATIC)
target_include_directories(IkakoRobomas PUBLIC
  ./IkakoRobomas
)
target_sources(IkakoRobomas PRIVATE
  ./IkakoRobomas/ikako_m2006.cpp
  ./IkakoRobomas/ikako_m3508.cpp
  ./IkakoRobomas/ikako_robomas.cpp
)
target_link_libraries(IkakoRobomas PRIVATE
  mbed-os ikarashiCAN_mk2 MotorController
)

#
# ikako_c620
#
add_library(ikako_c620 STATIC)
target_include_directories(ikako_c620 PUBLIC
  ./ikako_c620
)
target_sources(ikako_c620 PRIVATE
  ikako_c620/ikako_c620.cpp
)
target_link_libraries(ikako_c620 PRIVATE
  mbed-os ikarashiCAN_mk2
)

#
# bno055
#
add_library(bno055 STATIC)
target_include_directories(bno055 PUBLIC
  ./bno055
)
target_sources(bno055 PRIVATE
  bno055/bno055.cpp
)
target_link_libraries(bno055 PRIVATE
  mbed-os
)

#
# ikarashiCAN_mk2
#
add_library(ikarashiCAN_mk2 STATIC)
target_include_directories(ikarashiCAN_mk2 PUBLIC
  ./ikarashiCAN_mk2
  ./ikarashiCAN_mk2/NoMutexCAN-master
)
target_sources(ikarashiCAN_mk2 PRIVATE
  ikarashiCAN_mk2/ikarashiCAN_mk2.cpp
)
target_link_libraries(ikarashiCAN_mk2 PRIVATE
  mbed-os
)

#
# can_servo
#
add_library(can_servo STATIC)
target_include_directories(can_servo PUBLIC
  ./can_servo
)
target_sources(can_servo PRIVATE
  can_servo/can_servo.cpp
)
target_link_libraries(can_servo PRIVATE
  mbed-os
  ikarashiCAN_mk2
)

#
# ikakoMDC
#
add_library(ikakoMDC STATIC)
target_include_directories(ikakoMDC PUBLIC
  ./ikakoMDC
  ./ikakoMDC/lpf
  ./ikakoMDC/PID
)
target_sources(ikakoMDC PRIVATE
  ikakoMDC/ikakoMDC.cpp
  ikakoMDC/lpf/lpf.cpp
  ikakoMDC/PID/PID.cpp
)
target_link_libraries(ikakoMDC PRIVATE
  mbed-os
  ikarashiCAN_mk2
)

project(ControllerSTM32)

add_subdirectory(src)

mbed_finalize_build()