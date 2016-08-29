# -*- Mode: CMake -*-
#
# CMake build file for the RGBConverter algorithm
#

# Production specification for the RGBConverter algorithm
#
add_algorithm(RGBConverter RGBConverter.cc)

target_link_libraries(RGBConverter)
