add_executable(ControllerSTM32 main.cpp hcd.cpp hc.cpp endpoint.cpp device.cpp)


target_link_libraries(ControllerSTM32 PRIVATE mbed-os syoch-robotics)
mbed_set_post_build(ControllerSTM32)