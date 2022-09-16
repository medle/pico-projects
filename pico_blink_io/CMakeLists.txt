
set(PROJECT "blink_io")

add_executable(${PROJECT})

# must match with executable name and source file names
target_sources(${PROJECT} PRIVATE blink_io.c)

target_link_libraries(${PROJECT} pico_stdlib)

pico_enable_stdio_usb(${PROJECT} 1)
pico_enable_stdio_uart(${PROJECT} 0)

# create map/bin/hex file etc
pico_add_extra_outputs(${PROJECT})

