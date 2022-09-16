
add_executable(blink)

# must match with executable name and source file names
target_sources(blink PRIVATE blink.c)

target_link_libraries(blink pico_stdlib)

# create map/bin/hex file etc
pico_add_extra_outputs(blink)

