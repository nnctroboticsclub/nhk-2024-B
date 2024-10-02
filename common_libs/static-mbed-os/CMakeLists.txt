cmake_minimum_required(VERSION 3.19)
cmake_policy(VERSION 3.19)

set(MBED_APP_JSON_PATH mbed_app.json)

include(mbed-os/tools/cmake/app.cmake)
add_subdirectory(mbed-os)

project(static-mbed-os)

add_library(static-mbed-os-${MBED_TARGET} STATIC source/mbed-os.cpp)
target_link_libraries(static-mbed-os-${MBED_TARGET} PRIVATE
  mbed-os
)

install(TARGETS static-mbed-os-${MBED_TARGET}
  EXPORT static-mbed-os-${MBED_TARGET}-export
  ARCHIVE DESTINATION lib
  INCLUDES DESTINATION include
  PUBLIC_HEADER DESTINATION include
)


# -----

get_target_property(MBED_OS_INCLUDE_INTERNAL mbed-os INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "MBED_OS_INCLUDE_INTERNAL: ${MBED_OS_INCLUDE_INTERNAL}")

foreach(dir ${MBED_OS_INCLUDE_INTERNAL})
  if(NOT EXISTS ${dir})
    list(REMOVE_ITEM MBED_OS_INCLUDE_INTERNAL ${dir})
  endif()
endforeach()
file(WRITE ${CMAKE_BINARY_DIR}/mbed-os-include-internal.txt "${MBED_OS_INCLUDE_INTERNAL}")

# -----

get_target_property(MBED_OS_DEFINITIONS mbed-os INTERFACE_COMPILE_DEFINITIONS)
get_target_property(MBED_OS_DEFINITIONS_2 mbed-os DEFINITIONS)

file(WRITE ${CMAKE_BINARY_DIR}/mbed-os-definitions.txt "${MBED_OS_DEFINITIONS} ${MBED_OS_DEFINITIONS_2} __MBED__")

mbed_finalize_build()