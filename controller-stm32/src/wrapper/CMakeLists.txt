add_library(wrapper STATIC)
target_include_directories(wrapper PUBLIC
  inc
)
target_sources(wrapper PRIVATE
  src/wrapper.cpp
)
target_link_libraries(wrapper PRIVATE
  mbed-os syoch-robotics
)