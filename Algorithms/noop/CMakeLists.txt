# -*- Mode: CMake -*-
#
# CMake build file for the NOOP algorithm
#

# Production specification for the NOOP algorithm
#
add_algorithm(NOOP NOOP.cc)

target_link_libraries(NOOP)

add_unit_test(NOOPTest.cc NOOP)
