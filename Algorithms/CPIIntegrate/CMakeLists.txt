# -*- Mode: CMake -*-
# 
# CMake build file for the BinaryOp algorithm
# 

# Production specification for CPIIntegrate algorithm
#
add_algorithm(CPIIntegrate 
	      	  CPIIntegrate.cc)

target_link_libraries(CPIIntegrate)

# add_unit_test(CPIIntegrateTest.cc CPIIntegrate)
