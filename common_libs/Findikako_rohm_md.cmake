include(FetchContent)

FetchContent_Populate(ikako_rohm_md
  GIT_REPOSITORY git@github.com:nnctroboticsclub/ikako_rohm_md.git
  GIT_TAG a39a09aa3014ed3c19aa81cfdbb309d3503aa98b
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ikako_rohm_md
  REQUIRED_VARS
    ikako_rohm_md_SOURCE_DIR
)

if(ikako_rohm_md_FOUND AND NOT TARGET ikako_rohm_md)
  add_library(ikako_rohm_md STATIC)
  target_include_directories(ikako_rohm_md PUBLIC
    ${ikako_rohm_md_SOURCE_DIR}
  )
  target_sources(ikako_rohm_md PUBLIC
    ${ikako_rohm_md_SOURCE_DIR}/rohm_md.cpp
  )
  target_link_libraries(ikako_rohm_md PUBLIC
    static-mbed-os
    ikarashiCAN_mk2
  )
endif()


