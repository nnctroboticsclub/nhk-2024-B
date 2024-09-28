find_path(ikarashiCAN_mk2_DIR ikarashiCAN_mk2.h
  PATHS
    ENV ROBOCON_ROOT
    ENV ROBOCON_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
    /usr
    /usr/local
  PATH_SUFFIXES
    include
    ikarashiCAN_mk2
)
mark_as_advanced(
  ikarashiCAN_mk2_DIR
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ikarashiCAN_mk2
  REQUIRED_VARS
    ikarashiCAN_mk2_DIR
)

if(ikarashiCAN_mk2_FOUND AND NOT TARGET ikarashiCAN_mk2)
  add_library(ikarashiCAN_mk2 STATIC)
  target_include_directories(ikarashiCAN_mk2 PUBLIC
    ${ikarashiCAN_mk2_DIR}
    ${ikarashiCAN_mk2_DIR}/NoMutexCAN-master
  )
  target_sources(ikarashiCAN_mk2 PUBLIC
    ${ikarashiCAN_mk2_DIR}/ikarashiCAN_mk2.cpp
  )
  target_link_libraries(ikarashiCAN_mk2 PUBLIC
    static-mbed-os
  )
endif()


