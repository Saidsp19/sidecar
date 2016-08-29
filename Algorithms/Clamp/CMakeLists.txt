# -*- Mode: CMake -*-
# 
# CMake build file for the Clamp algorithm

# Production specification for Clamp algorithm
#
add_algorithm(Clamp Clamp.cc)

# Unit tests for Clamp
#
add_unit_test(ClampTest.cc Clamp)
