# -*- Mode: CMake -*-
#
# CMake build file for the DynamicThreshold algorithm
#

# Production specification for the DynamicThreshold algorithm
#
add_algorithm(DynamicThreshold 
			  DynamicThreshold.cc)

target_link_libraries(DynamicThreshold)

add_unit_test(DynamicThresholdTest.cc DynamicThreshold)
