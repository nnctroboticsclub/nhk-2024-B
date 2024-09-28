find_path(Futaba_Puropo_DIR puropo.h
  PATHS
    ENV ROBOCON_ROOT
    ENV ROBOCON_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}
    /usr
    /usr/local
  PATH_SUFFIXES
    include
    Futaba_Puropo
)
mark_as_advanced(
  Futaba_Puropo_DIR
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Futaba_Puropo
  REQUIRED_VARS
    Futaba_Puropo_DIR
)

if(Futaba_Puropo_FOUND AND NOT TARGET Futaba_Puropo)
  add_library(Futaba_Puropo STATIC)
  target_include_directories(Futaba_Puropo PUBLIC
    ${Futaba_Puropo_DIR}
  )
  target_sources(Futaba_Puropo PUBLIC
    ${Futaba_Puropo_DIR}/puropo.cpp
  )
  target_link_libraries(Futaba_Puropo PUBLIC
    static-mbed-os
  )
endif()


