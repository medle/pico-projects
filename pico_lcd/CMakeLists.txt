
add_executable(pico_lcd)

# must match with executable name and source file names
target_sources(pico_lcd PRIVATE pico_lcd.c)

target_link_libraries(pico_lcd pico_stdlib)

# create map/bin/hex file etc
pico_add_extra_outputs(pico_lcd)

