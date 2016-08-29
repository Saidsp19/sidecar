# -*- Mode: CMake -*-
#
# CMake build file for the scale algorithm
#

# Production specification for the scale algorithm
#
add_algorithm(Scale Scale.cc)

target_link_libraries(Scale)

add_unit_test(ScaleTest.cc Scale)
