
add_executable(blink2)

# must match with executable name and source file names
target_sources(blink2 PRIVATE blink2.c)

target_link_libraries(blink2 pico_stdlib)

# create map/bin/hex file etc
pico_add_extra_outputs(blink2)

