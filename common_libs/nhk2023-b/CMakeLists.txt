add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/libnhk2024b.a"
  COMMAND /home/vscode/.cargo/bin/cargo build
  COMMAND cp target/thumbv7em-none-eabi/debug/libnhk2024b.a "${CMAKE_BINARY_DIR}/libnhk2024b.a"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_target(nhk2024b_rs_target DEPENDS "${CMAKE_BINARY_DIR}/libnhk2024b.a")

add_library(nhk2024b_rs STATIC IMPORTED GLOBAL)
add_dependencies(nhk2024b_rs nhk2024b_rs_target)
set_target_properties(nhk2024b_rs
    PROPERTIES
    IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/libnhk2024b.a"
)

add_library(NHK2024B STATIC)
target_include_directories(NHK2024B PUBLIC
  ./inc
)
target_sources(NHK2024B PRIVATE)
target_link_libraries(NHK2024B PUBLIC
  mbed-os PS4_RX nhk2024b_rs
)

