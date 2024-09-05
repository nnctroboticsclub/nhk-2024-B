cmake_minimum_required(VERSION 3.19)
cmake_policy(VERSION 3.19)

set(MBED_APP_JSON_PATH mbed_app.json)

include(mbed-os/tools/cmake/app.cmake)
add_subdirectory(mbed-os)
add_subdirectory(ArduinoUsbHostShield)

project(app)

add_subdirectory(src)

mbed_finalize_build()