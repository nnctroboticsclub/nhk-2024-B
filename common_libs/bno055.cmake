add_library(bno055 OBJECT)
target_include_directories(bno055 PUBLIC
  ./bno055
)
target_sources(bno055 PUBLIC
  bno055/bno055.cpp
)
target_link_libraries(bno055 PRIVATE
  mbed-os
)
