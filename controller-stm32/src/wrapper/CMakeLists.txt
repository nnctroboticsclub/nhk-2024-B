add_library(wrapper OBJECT)
target_include_directories(wrapper PUBLIC
  inc
)
target_sources(wrapper PUBLIC
  src/wrapper.cpp
)
target_link_libraries(wrapper PRIVATE
  mbed-os syoch-robotics
)