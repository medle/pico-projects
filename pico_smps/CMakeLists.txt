
set(PROJECT "pico_smps")

add_executable(${PROJECT})
 
pico_generate_pio_header(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/smps_repeater.pio)

include_directories(../lib_ssd1306)

# must match with executable name and source file names
target_sources(${PROJECT} PRIVATE 
    smps_main.cpp 
    smps_core1.cpp
    smps_pio.cpp
    smps_display.cpp
    smps_config.cpp
    smps_util.cpp
    easy_pwm.c 
    easy_eeprom.c 
    easy_buttons.c)

target_link_libraries(${PROJECT} 
    pico_stdlib 
    pico_multicore
    hardware_i2c 
    hardware_pwm
    hardware_adc
    hardware_gpio
    hardware_adc
    hardware_pio
    SSD1306	   
    pico_bootsel_via_double_reset)

pico_enable_stdio_usb(${PROJECT} 1)
pico_enable_stdio_uart(${PROJECT} 0)

# create map/bin/hex file etc
pico_add_extra_outputs(${PROJECT})

# Entering the bootloader in this way also lets us specify a GPIO to be used
# as a bootloader activity LED:
target_compile_definitions(
	${PROJECT} PRIVATE
	PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED=25
	)
