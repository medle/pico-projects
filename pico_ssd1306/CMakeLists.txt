
set(PROJECT "pico_ssd1306")

add_executable(${PROJECT})

include_directories(../lib_ssd1306)

# must match with executable name and source file names
target_sources(${PROJECT} PRIVATE demo_ssd1306.cpp)

target_link_libraries(${PROJECT} 
    pico_stdlib 
    hardware_i2c 
    SSD1306	   
    pico_bootsel_via_double_reset)

pico_enable_stdio_usb(${PROJECT} 1)
pico_enable_stdio_uart(${PROJECT} 0)

# create map/bin/hex file etc
pico_add_extra_outputs(${PROJECT})

# Entering the bootloader in this way also lets us specify a GPIO to be used
# as a bootloader activity LED:
target_compile_definitions(
	pico_ssd1306 PRIVATE
	PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED=25
	)
