# Finds all source files in the current directory
# and save the name to the DIR_SSD1306_SRCS variable
aux_source_directory(. DIR_SSD1306_SRCS)

# Generate the link library
add_library(SSD1306 ${DIR_SSD1306_SRCS})
target_link_libraries(SSD1306 PUBLIC pico_stdlib hardware_i2c)

