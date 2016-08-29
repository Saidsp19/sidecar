# -*- Mode: CMake -*-
#
# CMake build file for the Difference algorithm
#

# Production specification for the Difference algorithm
#
add_algorithm(Difference Difference.cc)

target_link_libraries(Difference)

add_unit_test(DifferenceTest.cc Difference)
