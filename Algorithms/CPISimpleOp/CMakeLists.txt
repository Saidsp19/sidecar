# -*- Mode: CMake -*-
# 
# CMake build file for the CPISimpleOp algorithm
# 

# Production specification for CPISimpleOp algorithm
#
add_algorithm(CPISimpleOp 
	      	  CPISimpleOp.cc)

target_link_libraries(CPISimpleOp)

add_unit_test(CPISimpleOpTest.cc CPISimpleOp)
