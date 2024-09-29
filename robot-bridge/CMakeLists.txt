cmake_minimum_required(VERSION 3.19)
cmake_policy(VERSION 3.19)

set(MBED_APP_JSON_PATH ${CMAKE_SOURCE_DIR}/mbed_app.json)
include(mbed-os/tools/cmake/app.cmake)
add_subdirectory(mbed-os)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} /workspaces/nhk-2024-b/common_libs)


set(SYOCH_ROBOTICS_TARGET "Mbed" CACHE STRING "Thread implementation")
add_subdirectory(../common_libs common_lib)
include(../common_libs/static-mbed-os/library.cmake)

project(ControllerSTM32)

add_subdirectory(src)

mbed_finalize_build()