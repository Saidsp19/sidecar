# -*- Mode: CMake -*-
# 
# CMake build file for the SlidingOp algorithm
# 
# (C) Copyright 2010 Massachusetts Institute of Technology. All rights
# reserved.
#

#
# Production specification for SlidingOp algorithm
#
ADD_ALGORITHM( SlidingOp SlidingOp.cc )

#
# Unit test specification for SlidingOp algorithm
#
ADD_UNIT_TEST( SlidingOpTest.cc SlidingOp )
