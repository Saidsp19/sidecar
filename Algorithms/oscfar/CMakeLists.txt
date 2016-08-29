# -*- Mode: CMake -*-
#
# CMake build file for the OSCFAR algorithm
#

# Production specification for the OSCFAR algorithm
#
add_algorithm(OSCFAR OSCFAR.cc)

target_link_libraries(OSCFAR)

# add_unit_test(OSCFARTest.cc OSCFAR)
