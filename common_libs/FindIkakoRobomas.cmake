find_path(IkakoRobomas_DIR ikako_robomas.h
  PATHS
    ENV ROBOCON_ROOT
    ENV ROBOCON_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
    /usr
    /usr/local
  PATH_SUFFIXES
    include
    IkakoRobomas
)
mark_as_advanced(
  IkakoRobomas_DIR
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(IkakoRobomas
  REQUIRED_VARS
    IkakoRobomas_DIR
)

if(IkakoRobomas_FOUND AND NOT TARGET IkakoRobomas)
  add_library(IkakoRobomas STATIC)
  target_include_directories(IkakoRobomas PUBLIC
    ${IkakoRobomas_DIR}
  )
  target_sources(IkakoRobomas PUBLIC
    ${IkakoRobomas_DIR}/ikako_m2006.cpp
    ${IkakoRobomas_DIR}/ikako_m3508.cpp
    ${IkakoRobomas_DIR}/ikako_robomas.cpp
  )
  target_link_libraries(IkakoRobomas PUBLIC
    static-mbed-os
    ikarashiCAN_mk2
    MotorController
  )
endif()


