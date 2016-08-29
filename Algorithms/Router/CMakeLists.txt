# -*- Mode: CMake -*-
#
# CMake build file for the Router algorithm
#

# Production specification for the Router algorithm
#
add_algorithm(Router Router.cc)

target_link_libraries(Router)

add_unit_test(RouterTest.cc Router)
