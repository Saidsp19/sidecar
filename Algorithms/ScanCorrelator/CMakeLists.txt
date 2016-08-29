# -*- Mode: CMake -*-
#
# CMake build file for the ScanCorrelator algorithm
#

# Production specification for the ScanCorrelator algorithm
#
add_algorithm(ScanCorrelator ScanCorrelator.cc)

target_link_libraries(ScanCorrelator)

# add_unit_test(ScanCorrelatorTest.cc ScanCorrelator)
