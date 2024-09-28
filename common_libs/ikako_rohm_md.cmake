add_library(ikako_rohm_md OBJECT)
target_include_directories(ikako_rohm_md PUBLIC
  ./ikako_rohm_md
)
target_sources(ikako_rohm_md PRIVATE
  ikako_rohm_md/rohm_md.cpp
)
target_link_libraries(ikako_rohm_md PRIVATE
  mbed-os
  ikarashiCAN_mk2
)