find_path(can_servo_DIR can_servo.h
  PATHS
    ENV ROBOCON_ROOT
    ENV ROBOCON_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
    /usr
    /usr/local
  PATH_SUFFIXES
    include
    can_servo
)
mark_as_advanced(
  can_servo_DIR
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(can_servo
  REQUIRED_VARS
    can_servo_DIR
)

find_package(ikarashiCAN_mk2 REQUIRED)

if(can_servo_FOUND AND NOT TARGET can_servo)
  add_library(can_servo STATIC)
  target_include_directories(can_servo PUBLIC
    ${can_servo_DIR}
  )
  target_sources(can_servo PUBLIC
    ${can_servo_DIR}/can_servo.cpp
  )
  target_link_libraries(can_servo PUBLIC
    static-mbed-os
    ikarashiCAN_mk2
  )
endif()


