# -*- Mode: CMake -*-
#
# CMake build file for the EdgeDetector algorithm
#

# Production specification for the EdgeDetector algorithm
#
add_algorithm(EdgeDetector EdgeDetector.cc)

target_link_libraries(EdgeDetector)

add_unit_test(EdgeDetectorTest.cc EdgeDetector)
