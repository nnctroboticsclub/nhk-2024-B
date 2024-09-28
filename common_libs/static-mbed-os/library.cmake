add_library(static-mbed-os STATIC ${CMAKE_CURRENT_LIST_DIR}/source/mbed-os.cpp)
# set_target_properties(static-mbed-os PROPERTIES
#   IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/BUILD/NUCLEO_F446RE/libstatic-mbed-os.a
# )



get_target_property(MBED_OS_INCLUDE_INTERNAL mbed-os INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "MBED_OS_INCLUDE_INTERNAL: ${MBED_OS_INCLUDE_INTERNAL}")

foreach(dir ${MBED_OS_INCLUDE_INTERNAL})
if(NOT EXISTS ${dir})
list(REMOVE_ITEM MBED_OS_INCLUDE_INTERNAL ${dir})
endif()
endforeach()

target_include_directories(static-mbed-os INTERFACE ${MBED_OS_INCLUDE_INTERNAL})



get_target_property(MBED_OS_DEFINITIONS mbed-os INTERFACE_COMPILE_DEFINITIONS)
get_target_property(MBED_OS_DEFINITIONS_2 mbed-os DEFINITIONS)

target_compile_definitions(static-mbed-os INTERFACE
  ${MBED_OS_DEFINITIONS} ${MBED_OS_DEFINITIONS_2}
  __MBED__
)