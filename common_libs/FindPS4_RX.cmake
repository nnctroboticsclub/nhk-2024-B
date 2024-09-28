find_path(PS4_RX_DIR PS4.h
  PATHS
    ENV ROBOCON_ROOT
    ENV ROBOCON_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
    /usr
    /usr/local
  PATH_SUFFIXES
    include
    PS4_RX
)
mark_as_advanced(
  PS4_RX_DIR
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PS4_RX
  REQUIRED_VARS
    PS4_RX_DIR
)

if(PS4_RX_FOUND AND NOT TARGET PS4_RX)
  add_library(PS4_RX STATIC)
  target_include_directories(PS4_RX PUBLIC
    ${PS4_RX_DIR}
  )
  target_sources(PS4_RX PUBLIC
    ${PS4_RX_DIR}/PS4.cpp
  )
  target_link_libraries(PS4_RX PUBLIC
    static-mbed-os
  )
endif()


