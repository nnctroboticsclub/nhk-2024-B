cmake_minimum_required(VERSION 3.19)
cmake_policy(VERSION 3.19)

set(CMAKE_CXX_STANDARD 23 CACHE STRING "C++ standard")
set(MBED_APP_JSON_PATH ${CMAKE_SOURCE_DIR}/mbed_app.json)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  /workspaces/nhk-2024-b/common_libs
  /workspaces/nhk-2024-b/common_libs/syoch-robotics/libs/cmake
  /usr/arm-none-eabi/local/share/cmake
)

include(UseStaticMbedOS)
project(NHK)

set(SYOCH_ROBOTICS_TARGET "Mbed" CACHE STRING "Thread implementation")
add_subdirectory(common_libs)

add_subdirectory(connection-test)
add_subdirectory(robot1-main)
add_subdirectory(robot2-bridge)
add_subdirectory(robot3-collect)