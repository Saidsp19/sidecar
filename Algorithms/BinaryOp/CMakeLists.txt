# -*- Mode: CMake -*-
# 
# CMake build file for the BinaryOp algorithm
# 

# Production specification for BinaryOp algorithm
#
add_algorithm(BinaryOp BinaryOp.cc)

# Unit tests for BinaryOp
#
add_unit_test(BinaryOpTest.cc BinaryOp)
