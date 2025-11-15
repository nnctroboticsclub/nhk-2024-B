add_library(r2-robomas-ikako INTERFACE)

target_include_directories(r2-robomas-ikako .)

target_link_libraries(r2-robomas-ikako PUBLIC ikarashiCAN_mk2 syoch-robotics-ikako-robomas)