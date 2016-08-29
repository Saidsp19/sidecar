# -*- Mode: CMake -*-
#
# CMake build file for the MTI2 algorithm
#

# Production specification for the MTI2 algorithm
#
add_algorithm(MTI2 MTI2.cc)

target_link_libraries(MTI2)

add_unit_test(MTI2Test.cc MTI2)
