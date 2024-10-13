# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-project-template)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  /workspaces/nhk-2024-b/common_libs
  /workspaces/nhk-2024-b/common_libs/syoch-robotics/libs/cmake
  /usr/arm-none-eabi/local/share/cmake
)

set(SYOCH_ROBOTICS_TARGET "ESP" CACHE STRING "Thread implementation")
set(NON_THREAD ON CACHE BOOL "Non-thread")
add_subdirectory(../common_libs common_libs)