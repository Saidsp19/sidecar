# -*- Mode: CMake -*-
# 
# CMake build file for the Despeckle algorithm
# 

# Production specification for Despeckle algorithm
#
add_algorithm(Despeckle 
	       	  Despeckle.cc)

target_link_libraries(Despeckle)

add_unit_test(DespeckleTests.cc Despeckle)
