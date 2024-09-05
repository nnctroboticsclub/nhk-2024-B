add_executable(app)
target_sources(app PRIVATE
  main.cpp
)

target_link_libraries(app PRIVATE
  mbed-os USB_Host_Shield
)
mbed_set_post_build(app)