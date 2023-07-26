
set(PROJECT "machine2")

add_executable(${PROJECT})

include_directories(../lib_lcd114)

# must match with executable name and source file names
target_sources(
	${PROJECT} PRIVATE 
        main.c
        util.c 
        draw.c
        pwm.c
        )

# Double tap reset into bootrom is injected by linking with the
# pico_bootsel_via_double_reset library
target_link_libraries(
        ${PROJECT}
        pico_stdlib
        pico_bootsel_via_double_reset
        LCD114
        hardware_spi
        hardware_i2c
        hardware_pwm 
        hardware_dma
        )

# Entering the bootloader in this way also lets us specify a GPIO to be used
# as a bootloader activity LED:
target_compile_definitions(${PROJECT} PRIVATE
	PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED=25
	)

pico_enable_stdio_usb(${PROJECT} 1)
pico_enable_stdio_uart(${PROJECT} 0)

# create map/bin/hex file etc
pico_add_extra_outputs(${PROJECT})

