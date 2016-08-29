# -*- Mode: CMake -*-
#
# CMake build file for the DownConverter algorithm
#

# Production specification for the DownConverter algorithm
#
add_algorithm(DownConverter 
			  Channel.cc 
	       	  DownConverter.cc)

target_link_libraries(DownConverter)

# add_unit_test(DownConverterTest.cc DownConverter)
