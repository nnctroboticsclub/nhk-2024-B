add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/libapp_rs.a"
  COMMAND /home/vscode/.cargo/bin/cargo build
  COMMAND cp target/thumbv7em-none-eabi/debug/libapp_rs.a "${CMAKE_BINARY_DIR}/libapp_rs.a"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_target(app_rs_target DEPENDS "${CMAKE_BINARY_DIR}/libapp_rs.a")

add_library(app_rs STATIC IMPORTED GLOBAL)
add_dependencies(app_rs app_rs_target)
set_target_properties(app_rs
    PROPERTIES
    IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/libapp_rs.a"
)