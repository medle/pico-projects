
add_executable(blink_print)

# must match with executable name and source file names
target_sources(blink_print PRIVATE blink_print.c)

target_link_libraries(blink_print pico_stdlib)

pico_enable_stdio_usb(blink_print 1)
pico_enable_stdio_uart(blink_print 0)

# create map/bin/hex file etc
pico_add_extra_outputs(blink_print)

