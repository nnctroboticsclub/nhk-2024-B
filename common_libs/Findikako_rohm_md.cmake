find_path(ikako_rohm_md_DIR rohm_md.h
  PATHS
    ENV ROBOCON_ROOT
    ENV ROBOCON_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
    /usr
    /usr/local
  PATH_SUFFIXES
    include
    ikako_rohm_md
)
mark_as_advanced(
  ikako_rohm_md_DIR
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ikako_rohm_md
  REQUIRED_VARS
    ikako_rohm_md_DIR
)

if(ikako_rohm_md_FOUND AND NOT TARGET ikako_rohm_md)
  add_library(ikako_rohm_md STATIC)
  target_include_directories(ikako_rohm_md PUBLIC
    ${ikako_rohm_md_DIR}
  )
  target_sources(ikako_rohm_md PUBLIC
    ${ikako_rohm_md_DIR}/rohm_md.cpp
  )
  target_link_libraries(ikako_rohm_md PUBLIC
    static-mbed-os
    ikarashiCAN_mk2
  )
endif()


