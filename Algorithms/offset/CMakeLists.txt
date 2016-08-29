# -*- Mode: CMake -*-
#
# CMake build file for the Offset algorithm
#

# Production specification for the Offset algorithm
#
add_algorithm(Offset Offset.cc)

target_link_libraries(Offset)

add_unit_test(OffsetTest.cc Offset)
