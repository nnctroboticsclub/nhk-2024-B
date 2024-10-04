message(STATUS "Finding for libstatic-mbed-os-${MBED_TARGET}.a")
message(STATUS "  at $ENV{ROBOCON_ROOT}")
message(STATUS "  at ${ROBOCON_ROOT}")
message(STATUS "  at /usr/arm-none-eabi/local")
find_file(StaticMbedOSArchive
  NAMES
    libstatic-mbed-os-${MBED_TARGET}.a
  PATHS
    ENV ROBOCON_ROOT
    ${ROBOCON_ROOT}
    /usr/arm-none-eabi/local
  PATH_SUFFIXES
    lib
)
message(STATUS "StaticMbedOSArchive: ${StaticMbedOSArchive}")
mark_as_advanced(StaticMbedOSArchive)


file(READ
  ${CMAKE_CURRENT_LIST_DIR}/mbed-os@3297bae/definitions.txt
  STATIC_MBED_OS_DEFINITIONS
)

file(READ
  ${CMAKE_CURRENT_LIST_DIR}/mbed-os@3297bae/includes.txt
  STATIC_MBED_OS_INCLUDES
)
string(REPLACE "\n" ";" STATIC_MBED_OS_INCLUDES ${STATIC_MBED_OS_INCLUDES})


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(StaticMbedOS REQUIRED_VARS
  StaticMbedOSArchive
)

if(StaticMbedOS_FOUND AND NOT TARGET StaticMbedOS)
  add_library(StaticMbedOS UNKNOWN IMPORTED)
  set_target_properties(StaticMbedOS PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
    IMPORTED_LOCATION "${StaticMbedOSArchive}"
  )

  target_include_directories(StaticMbedOS INTERFACE
    ${STATIC_MBED_OS_INCLUDES}
  )

  target_compile_definitions(StaticMbedOS INTERFACE
    ${STATIC_MBED_OS_DEFINITIONS}
  )
endif()
