# -*- Mode: CMake -*-
# 
# CMake build file for the Delay algorithm
# 

# Production specification for Delay algorithm
#
add_algorithm(Delay 
	       	  Delay.cc)

target_link_libraries(Delay)

add_unit_test(DelayTest.cc Delay)
