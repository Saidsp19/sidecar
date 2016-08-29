# -*- Mode: CMake -*-
# 
# CMake build file for the Decimator algorithm
# 

# Production specification for Decimator algorithm
#
add_algorithm(Decimator Decimator.cc)

target_link_libraries(Decimator)

add_unit_test(DecimatorTest.cc Decimator)
