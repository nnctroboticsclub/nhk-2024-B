cmake_minimum_required(VERSION 3.19)
cmake_policy(VERSION 3.19)

add_compile_options(-fdiagnostics-color=always)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  /workspaces/nhk-2024-b/common_libs
  /workspaces/nhk-2024-b/common_libs/syoch-robotics/libs/cmake
)

set(STM32CUBE_TARGET NUCLEO_F446RE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(MBED_APP_JSON_PATH ${CMAKE_SOURCE_DIR}/mbed_app.json)
set(SYOCH_ROBOTICS_TARGET "Mbed" CACHE STRING "Thread implementation")
set(EMULATION_ENABLED OFF CACHE STRING "Enable emulation")
set(MBED_TARGET "NUCLEO_F446RE" CACHE STRING "Mbed target")

include(/usr/arm-none-eabi/share/cmake/StaticMbedOSToolchain-NUCLEO_F446RE.cmake)

project(NHK)
add_subdirectory(common_libs)

# add_subdirectory(connection-test)
add_subdirectory(src/robot1-main)
add_subdirectory(src/robot2-bridge)
add_subdirectory(src/robot3-collect)