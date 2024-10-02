include(FetchContent)

FetchContent_Populate(mbed-os
  GIT_REPOSITORY git@github.com:mbed-ce/mbed-os.git
  GIT_TAG 3297baedff11f68657f656d17ec738ee59ea3e8f
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/.projects/mbed-os/src
  BINARY_DIR ${CMAKE_CURRENT_LIST_DIR}/.projects/mbed-os/build
  SUBBUILD_DIR ${CMAKE_CURRENT_LIST_DIR}/.projects/mbed-os/subbuild
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(mbed-os
  REQUIRED_VARS
    mbed-os_SOURCE_DIR
)