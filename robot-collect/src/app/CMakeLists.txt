add_executable(app)
target_sources(app PRIVATE
  main.cpp
)

target_link_libraries(app PRIVATE
  mbed-os
  syoch-robotics-logger
  syoch-robotics-mbed-uart
  syoch-robotics-gpio
  fep
  bd621x
  ikarashiCAN_mk2
  PS4_RX
  NHK2024B
  Futaba_Puropo
)

mbed_set_post_build(app)