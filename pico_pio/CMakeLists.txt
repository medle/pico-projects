
set(PROJECT "pico_pio")

add_executable(${PROJECT})

pico_generate_pio_header(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/pwm.pio)
pico_generate_pio_header(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/limiter.pio)

target_sources(${PROJECT} PRIVATE pwm.c)

pico_enable_stdio_usb(${PROJECT} 1)
pico_enable_stdio_uart(${PROJECT} 0)

target_link_libraries(
	${PROJECT} PRIVATE 
	pico_stdlib 
	hardware_pio
	pico_bootsel_via_double_reset)

pico_add_extra_outputs(${PROJECT})

# Entering the bootloader in this way also lets us specify a GPIO to be used
# as a bootloader activity LED:
target_compile_definitions(
	${PROJECT} PRIVATE
	PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED=25)

