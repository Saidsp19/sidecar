# -*- Mode: CMake -*-
#
# CMake build file for the Segment2Extraction algorithm
#

# Production specification for the Segment2Extraction algorithm
#
add_algorithm(Segment2Extraction Segment2Extraction.cc)

target_link_libraries(Segment2Extraction)
