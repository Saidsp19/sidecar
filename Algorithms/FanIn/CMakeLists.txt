# -*- Mode: CMake -*-
#
# CMake build file for the FanIn algorithm
#

# Production specification for the FanIn algorithm
#
add_algorithm(FanIn FanIn.cc)

target_link_libraries(FanIn)

add_unit_test(FanInTest.cc FanIn)
