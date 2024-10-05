cmake_minimum_required(VERSION 3.19)
cmake_policy(VERSION 3.19)

# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} /workspaces/nhk-2024-b/common_libs)
set(CMAKE_INSTALL_MESSAGE LAZY)
set(MBED_APP_JSON_PATH mbed_app.json)
set(MBED_TOOLCHAIN GCC_ARM)

include(mbed-os/tools/cmake/app.cmake)
add_subdirectory(mbed-os)

project(static-mbed-os)

# ----- Target

add_library(static-mbed-os-${MBED_TARGET} STATIC
  source/mbed-os.cpp
)
target_link_libraries(static-mbed-os-${MBED_TARGET} PUBLIC
  mbed-os
)
install(
  TARGETS static-mbed-os-${MBED_TARGET}
  ARCHIVE DESTINATION share/cmake/mbed-os@3297bae/
)

# ---- Install Headers

get_target_property(MBED_OS_INCLUDE1 mbed-os         INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(MBED_OS_INCLUDE3 mbed-core-flags INTERFACE_INCLUDE_DIRECTORIES)

set(MBED_OS_INCLUDES
  ${MBED_OS_INCLUDE1}
  ${MBED_OS_INCLUDE3}
)
list(REMOVE_DUPLICATES MBED_OS_INCLUDES)

foreach(dir ${MBED_OS_INCLUDES})
  if(NOT EXISTS ${dir})
    list(REMOVE_ITEM MBED_OS_INCLUDES ${dir})
    message(WARNING "Directory not found: ${dir}")
  endif()
endforeach()

list(JOIN MBED_OS_INCLUDES "\n" MBED_OS_INCLUDES)
file(WRITE ${CMAKE_BINARY_DIR}/includes.txt "${MBED_OS_INCLUDES}")

install(
  FILES ${CMAKE_BINARY_DIR}/includes.txt
  DESTINATION share/cmake/mbed-os@3297bae
)

# ----- Definitions

get_target_property(MBED_OS_DEFINITIONS mbed-os INTERFACE_COMPILE_DEFINITIONS)

list(JOIN MBED_OS_DEFINITIONS "\n" MBED_OS_DEFINITIONS)

file(WRITE ${CMAKE_BINARY_DIR}/definitions.txt "${MBED_OS_DEFINITIONS}")
install(
  FILES ${CMAKE_BINARY_DIR}/definitions.txt
  DESTINATION share/cmake/mbed-os@3297bae
)

# ----- Definitions

get_target_property(MBED_OS_LINKER_PATH mbed-os LINKER_SCRIPT_PATH)
install(
  FILES ${MBED_OS_LINKER_PATH}
  DESTINATION share/cmake/mbed-os@3297bae
  RENAME linker_script.ld
)

# ----- Find Package
install(
  FILES cmake/FindStaticMbedOS.cmake
  DESTINATION share/cmake
)
install(
  FILES cmake/UseStaticMbedOS.cmake
  DESTINATION share/cmake
)