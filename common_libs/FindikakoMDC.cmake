find_path(ikakoMDC_DIR ikakoMDC.h
  PATHS
    ENV ROBOCON_ROOT
    ENV ROBOCON_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
    /usr
    /usr/local
  PATH_SUFFIXES
    include
    ikakoMDC
)
mark_as_advanced(
  ikakoMDC_DIR
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ikakoMDC
  REQUIRED_VARS
    ikakoMDC_DIR
)

find_package(ikarashiCAN_mk2 REQUIRED)

if(ikakoMDC_FOUND AND NOT TARGET ikakoMDC)
  add_library(ikakoMDC STATIC)
  target_include_directories(ikakoMDC PUBLIC
    ${ikakoMDC_DIR}
    ${ikakoMDC_DIR}/lpf
    ${ikakoMDC_DIR}/PID
  )
  target_sources(ikakoMDC PUBLIC
    ${ikakoMDC_DIR}/ikakoMDC.cpp
    ${ikakoMDC_DIR}/lpf/lpf.cpp
    ${ikakoMDC_DIR}/PID/PID.cpp
  )
  target_link_libraries(ikakoMDC PUBLIC
    static-mbed-os
    ikarashiCAN_mk2
  )
endif()


