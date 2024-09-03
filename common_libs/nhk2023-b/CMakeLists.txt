add_library(NHK2024B OBJECT)
target_include_directories(NHK2024B PUBLIC
  .
)
target_sources(NHK2024B PRIVATE)
target_link_libraries(NHK2024B PUBLIC
  mbed-os PS4_RX
)

