cmake_minimum_required(VERSION 3.19)

project(framework-test)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard")
set(SYOCH_ROBOTICS_TARGET "Emulation" CACHE STRING "syoch-robotics target")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  /usr/arm-none-eabi/local/share/cmake
)


add_compile_options(-fdiagnostics-color=always)
add_subdirectory(common_libs/syoch-robotics/libs)
add_subdirectory(common_libs/robobus)

find_package(fmt REQUIRED)

add_executable(framework-test
    main.cpp
)

target_link_libraries(framework-test
    syoch-robotics-common
    syoch-robotics-logger
    syoch-robotics-thread

    fmt::fmt
    robobus
)