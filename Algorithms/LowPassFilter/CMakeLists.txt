# -*- Mode: CMake -*-
#
# CMake build file for the LowPassFilter algorithm
#

# Production specification for the LowPassFilter algorithm
#
add_algorithm(LowPassFilter LowPassFilter.cc)

target_link_libraries(LowPassFilter ${MATHLIBS})

# add_unit_test(LowPassFilterTest.cc LowPassFilter)
