# -*- Mode: CMake -*-
#
# CMake build file for the FanOut algorithm
#

# Production specification for the FanOut algorithm
#
add_algorithm(FanOut FanOut.cc)

target_link_libraries(FanOut)

add_unit_test(FanOutTest.cc FanOut)
