cmake_minimum_required(VERSION 3.19)
cmake_policy(VERSION 3.19)

set(MBED_APP_JSON_PATH ${CMAKE_SOURCE_DIR}/mbed_app.json)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  /workspaces/nhk-2024-b/common_libs
  /workspaces/nhk-2024-b/common_libs/syoch-robotics/libs/cmake
  /usr/arm-none-eabi/local/share/cmake
)
include(MbedCE)

set(SYOCH_ROBOTICS_TARGET "Mbed" CACHE STRING "Thread implementation")
add_subdirectory(../common_libs common_lib)

project(NHK)
add_subdirectory(src)