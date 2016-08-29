# -*- Mode: CMake -*-
#
# CMake build file for the RawPRI algorithm
#

# Production specification for the RawPRI algorithm
#
add_algorithm(RawPRI RawPRI.cc)

target_link_libraries(RawPRI)

# add_unit_test(RawPRITest.cc RawPRI)
