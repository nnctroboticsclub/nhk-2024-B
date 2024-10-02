find_package(mbed-os REQUIRED)

message(STATUS "mbed-os_SOURCE_DIR: ${mbed-os_SOURCE_DIR}")

include(${mbed-os_SOURCE_DIR}/tools/cmake/app.cmake)
add_subdirectory(${mbed-os_SOURCE_DIR} mbed-os)

include(../common_libs/static-mbed-os/library.cmake)

message(STATUS "@@@@@ [MBed] CMAKE_CXX_ARCHIVE_CREATE: ${CMAKE_CXX_ARCHIVE_CREATE}")
message(STATUS "@@@@@ [MBed] CMAKE_CXX_COMPILE_OBJECT: ${CMAKE_CXX_COMPILE_OBJECT}")
message(STATUS "@@@@@ [MBed] CMAKE_CXX_ARCHIVE_FINISH: ${CMAKE_CXX_ARCHIVE_FINISH}")