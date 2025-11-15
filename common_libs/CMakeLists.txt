set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} /workspaces/nhk-2024-b/common_libs /usr/arm-none-eabi/local/share/cmake)

LIST(APPEND SYOCH_ROBOTICS_BYPASS_DEPS "IkakoRobomas")
set(SYOCH_ROBOTICS_DRIVER_GPIO ON CACHE INTERNAL "")
SET(SYOCH_ROBOTICS_IKAKO_ROBOMAS OFF CACHE INTERNAL "")

add_subdirectory(robobus)
add_subdirectory(nhk2023-b)
add_subdirectory(syoch-robotics/libs EXCLUDE_FROM_ALL)