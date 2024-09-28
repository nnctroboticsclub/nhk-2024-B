add_library(ikako_c620 OBJECT)
target_include_directories(ikako_c620 PUBLIC
  ./ikako_c620
)
target_sources(ikako_c620 PUBLIC
  ikako_c620/ikako_c620.cpp
)
target_link_libraries(ikako_c620 PRIVATE
  mbed-os ikarashiCAN_mk2
)