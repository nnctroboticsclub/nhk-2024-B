find_path(MotorController_DIR MotorController.h
  PATHS
    ENV ROBOCON_ROOT
    ENV ROBOCON_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
    /usr
    /usr/local
  PATH_SUFFIXES
    include
    MotorController
)
mark_as_advanced(
  MotorController_DIR
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MotorController
  REQUIRED_VARS
    MotorController_DIR
)

if(MotorController_FOUND AND NOT TARGET MotorController)
  add_library(MotorController STATIC)
  target_include_directories(MotorController PUBLIC
    ${MotorController_DIR}
    ${MotorController_DIR}/Ikako_PID
    ${MotorController_DIR}/DisturbanceObserver
    ${MotorController_DIR}/DisturbanceObserver/LowPassFilter
  )
  target_sources(MotorController PUBLIC
    ${MotorController_DIR}/MotorController.cpp
    ${MotorController_DIR}/Ikako_PID/ikako_PID.cpp
    ${MotorController_DIR}/DisturbanceObserver/DOB.cpp
    ${MotorController_DIR}/DisturbanceObserver/LowPassFilter/LowPassFilter.cpp
  )
  target_link_libraries(MotorController PUBLIC
    static-mbed-os
    ikarashiCAN_mk2
  )
endif()


