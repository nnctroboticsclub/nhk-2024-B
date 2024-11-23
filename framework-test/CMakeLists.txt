cmake_minimum_required(VERSION 3.19)

project(framework-test)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard")
set(SYOCH_ROBOTICS_TARGET "Emulation" CACHE STRING "syoch-robotics target")

add_subdirectory(common_libs/syoch-robotics/libs)


add_executable(framework-test
    main.cpp
)

target_link_libraries(framework-test
    syoch-robotics-logger
    syoch-robotics-thread
)