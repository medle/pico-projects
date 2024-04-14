 
cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
 
# Initialize the SDK
pico_sdk_init()

# Add multi example
add_subdirectory(lib_lcd114)
add_subdirectory(lib_ssd1306)
add_subdirectory(pico_blink)
add_subdirectory(pico_blink_print)
add_subdirectory(pico_blink_io)
add_subdirectory(pico_pwm)
add_subdirectory(pico_adc)
add_subdirectory(pico_machine)
add_subdirectory(pico_lcd)
add_subdirectory(pico_i2c_scan)
add_subdirectory(machine2)
add_subdirectory(demo_ssd1306)
add_subdirectory(pico_rectifier)

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )