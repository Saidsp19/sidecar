# -*- Mode: CMake -*-
# 
# CMake build file for the CFAR algorithm
# 
# (C) Copyright 2009 Massachusetts Institute of Technology. All rights
# reserved.
#

add_algorithm( CFAR CFAR.cc )

target_link_libraries( CFAR )
