add_executable(app)
target_sources(app PRIVATE
  app.cpp
  communication.cpp
  main.cpp
  project/driving_can.cpp
)

target_link_libraries(app PRIVATE
  mbed-os syoch-robotics ikarashiCAN_mk2 MotorController IkakoRobomas ikarashiCAN_mk2
)

mbed_set_post_build(app)