find_package(StaticMbedOS REQUIRED)

add_library(dummy-mbed-os INTERFACE)
target_link_libraries(dummy-mbed-os INTERFACE StaticMbedOS)

set_target_properties(dummy-mbed-os PROPERTIES
  LINKER_SCRIPT_PATH ${CMAKE_CURRENT_LIST_DIR}/mbed-os@3297bae/linker_script.ld
)