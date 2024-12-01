cmake_minimum_required(VERSION 3.19)

project(framework-test)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard")
set(SYOCH_ROBOTICS_TARGET "Emulation" CACHE STRING "syoch-robotics target")


include(FetchContent)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        e69e5f977d458f2650bb346dadf2ad30c5320281) # 10.2.1
FetchContent_MakeAvailable(fmt)


add_compile_options(-fdiagnostics-color=always)
add_subdirectory(common_libs/syoch-robotics/libs)
add_subdirectory(common_libs/framework)

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