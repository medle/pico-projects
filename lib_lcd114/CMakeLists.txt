# Finds all source files in the current directory
# and save the name to the DIR_Fonts_SRCS)variable
aux_source_directory(. DIR_LCD114_SRCS)

# Generate the link library
add_library(LCD114 ${DIR_LCD114_SRCS})
target_link_libraries(LCD114 PUBLIC pico_stdlib hardware_spi hardware_i2c hardware_pwm)