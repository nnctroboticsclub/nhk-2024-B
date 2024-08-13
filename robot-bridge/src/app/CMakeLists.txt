add_executable(app)
target_sources(app PRIVATE
  main.cpp
)

target_link_libraries(app PRIVATE
  mbed-os NHK2024B PS4_RX
  syoch-robotics ikarashiCAN_mk2 MotorController
  IkakoRobomas ikako_rohm_md can_servo
)

mbed_set_post_build(app)