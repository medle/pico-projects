
add_executable(blinkprint)

# must match with executable name and source file names
target_sources(blinkprint PRIVATE blinkprint.c)

target_link_libraries(blinkprint pico_stdlib)

pico_enable_stdio_usb(blinkprint 1)
pico_enable_stdio_uart(blinkprint 0)

# create map/bin/hex file etc
pico_add_extra_outputs(blinkprint)

