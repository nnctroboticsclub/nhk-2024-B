cmake_minimum_required(VERSION 3.19)
cmake_policy(VERSION 3.19)

set(MBED_APP_JSON_PATH ${CMAKE_SOURCE_DIR}/mbed_app.json)
set(GRAPHVIZ_IGNORE_TARGETS "mbed.*")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} /workspaces/nhk-2024-b/common_libs /usr/arm-none-eabi/local/share/cmake)
include(MbedCE)

set(SYOCH_ROBOTICS_TARGET "Mbed" CACHE STRING "Thread implementation")
add_subdirectory(../common_libs common_lib)

project(ControllerSTM32)
# set(CMAKE_CXX_COMPILE_OBJECT "<CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>" CACHE FILEPATH "some")
# set(CMAKE_CXX_ARCHIVE_CREATE "${CMAKE_CXX_ARCHIVE_CREATE}" CACHE FILEPATH "some")
# sset(CMAKE_CXX_ARCHIVE_FINISH "${CMAKE_CXX_ARCHIVE_FINISH}" CACHE FILEPATH "some")
add_subdirectory(src)