# -*- Mode: CMake -*-
# 
# CMake build file for the CPIMarker algorithm
# 

# Production specification for CPIMarker algorithm
#
add_algorithm(CPIMarker CPIMarker.cc)

target_link_libraries(CPIMarker)

add_unit_test(CPIMarkerTest.cc CPIMarker)
