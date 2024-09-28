add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/libnhk2024b_rs.a"
  COMMAND /home/vscode/.cargo/bin/cargo build
  COMMAND cp target/thumbv7em-none-eabi/debug/libnhk2024b_rs.a "${CMAKE_BINARY_DIR}/libnhk2024b_rs.a"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_target(NHK2024BRsLib_Target DEPENDS "${CMAKE_BINARY_DIR}/libnhk2024b_rs.a")

add_library(NHK2024BRsLib STATIC IMPORTED GLOBAL)
add_dependencies(NHK2024BRsLib NHK2024BRsLib_Target)
set_target_properties(NHK2024BRsLib
    PROPERTIES
    IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/libnhk2024b_rs.a"
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../common_libs/srobo_base $(CMAKE_CURRENT_BINARY_DIR)/srobo_base)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../common_libs/im920_rs $(CMAKE_CURRENT_BINARY_DIR)/im920_rs)

add_library(NHK2024BRuntime STATIC
  cxx/mem.cpp
  cxx/logger.cpp
  cxx/ps4_con.cpp
)

target_include_directories(NHK2024BRuntime PUBLIC
  ./inc
)

target_link_libraries(NHK2024BRuntime PUBLIC
  syoch-robotics-logger
  syoch-robotics-common
  ssp
)

add_library(NHK2024B STATIC

)
target_link_libraries(NHK2024B PUBLIC
  static-mbed-os
  srobo_base_hxx
  im920_rs_hxx
  NHK2024BRsLib
  NHK2024BRuntime
  PS4_RX
)

