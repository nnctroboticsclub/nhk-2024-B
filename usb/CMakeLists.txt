cmake_minimum_required(VERSION 3.5)

message(STATUS "CMAKE_C_COMPILE_OBJECT: ${CMAKE_C_COMPILE_OBJECT}")
message(STATUS "CMAKE_CXX_COMPILE_OBJECT: ${CMAKE_CXX_COMPILE_OBJECT}")

# --------------------------
# Toolchain setup
# --------------------------

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN_PREFIX "arm-none-eabi")
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}-objcopy)
set(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}-objdump)
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}-size)


set(MY_ARCH_FLAGS "-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=soft -mthumb")
set(MY_COMPILE_FLAGS "-ffunction-sections -fdata-sections -O0 -g3")
set(MY_COMMON_FLAGS "${MY_ARCH_FLAGS} -specs=nosys.specs")

set(CMAKE_C_FLAGS "${MY_COMMON_FLAGS} ${MY_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${MY_COMMON_FLAGS} ${MY_COMPILE_FLAGS}")
set(CMAKE_ASM_FLAGS "${MY_COMMON_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS "${MY_COMMON_ARGS} -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/STM32F446RETX_FLASH.ld -Wl,--gc-sections")


enable_language(C CXX ASM)

# --------------------------
# Subdirectories
# --------------------------


set(syoch-robotics-disable-features ON)
set(syoch-robotics-disable-thread ON)
add_subdirectory(syoch-robotics)

add_subdirectory(nhk2023-b)

# --------------------------
# App
# --------------------------
project(app)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
  "Core/*.cpp"
  "Core/*.c"
  "Drivers/*.c"
  "Middlewares/*.c"
  "USB_HOST/*.c"
)

add_executable(app)
target_sources(app PRIVATE
  ${SOURCES}
  Core/Startup/startup_stm32f446retx.s
)
target_include_directories(app PRIVATE
  "Core/Inc"
  "STM32F4xx_HAL_Config/Inc"
  "Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc"
  "Middlewares/ST/STM32_USB_Host_Library/Core/Inc"
  "Middlewares/ST/Drivers/STM32F4xx_HAL_Driver/Inc"
  "Drivers/CMSIS/Include"
  "Drivers/CMSIS/Device/ST/STM32F4xx/Include"
  "Drivers/STM32F4xx_HAL_Driver/Inc"
  "USB_HOST/Target"
  "USB_HOST/App"
)
target_compile_definitions(app PRIVATE
  -DDEBUG
  -DUSE_HAL_DRIVER
  -DSTM32F446xx
)
target_link_libraries(app PRIVATE
  srobo_base_hxx
  im920_rs_hxx
  NHK2024BRsLib
  NHK2024BRuntime
  syoch-robotics
)

add_custom_target(app.hex DEPENDS app COMMAND ${CMAKE_OBJCOPY} -O ihex app app.hex)