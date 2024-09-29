include(FetchContent)

FetchContent_Populate(PS4_RX
  GIT_REPOSITORY git@github.com:nnctroboticsclub/PS4_RX.git
  GIT_TAG 0aca1751a597c3687bb994fff6c9b282ead224a0
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PS4_RX
  REQUIRED_VARS
    ps4_rx_SOURCE_DIR
)

if(PS4_RX_FOUND AND NOT TARGET PS4_RX)
  add_library(PS4_RX STATIC)
  target_include_directories(PS4_RX PUBLIC
    ${ps4_rx_SOURCE_DIR}
  )
  target_sources(PS4_RX PUBLIC
    ${ps4_rx_SOURCE_DIR}/PS4.cpp
  )
  target_link_libraries(PS4_RX PUBLIC
    static-mbed-os
  )
endif()


