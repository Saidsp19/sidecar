# -*- Mode: CMake -*-
#
# CMake build file for the PRISegmentation algorithm
#

# Production specification for the PRISegmentation algorithm
#
add_algorithm(PRISegmentation PRISegmentation.cc)

target_link_libraries(PRISegmentation)

# add_unit_test(PRISegmentationTest.cc PRISegmentation)
