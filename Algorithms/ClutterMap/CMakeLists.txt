# -*- Mode: CMake -*-
# 
# CMake build file for the clutter map algorithm
#

# Production specification for ClutterMap algorithm
#
add_algorithm(ClutterMap 
			  ClutterMap.cc 
	      	  MapBuffer.cc)

target_link_libraries(ClutterMap)
