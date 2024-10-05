set(StaticMbedOSRoot ${CMAKE_CURRENT_LIST_DIR}/mbed-os@3297bae)

set(StaticMbedOSArchive
  ${StaticMbedOSRoot}/libstatic-mbed-os-${MBED_TARGET}.a
)


file(READ
  ${StaticMbedOSRoot}/definitions.txt
  STATIC_MBED_OS_DEFINITIONS
)
string(REPLACE "\n" ";" STATIC_MBED_OS_DEFINITIONS ${STATIC_MBED_OS_DEFINITIONS})

file(READ
  ${StaticMbedOSRoot}/includes.txt
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
  target_compile_definitions(StaticMbedOS INTERFACE
    __MBED__
  )
endif()
