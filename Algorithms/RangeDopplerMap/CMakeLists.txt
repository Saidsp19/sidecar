# -*- Mode: CMake -*-
#
# CMake build file for the RangeDopplerMap algorithm
#

# Production specification for the RangeDopplerMap algorithm
#
add_algorithm(RangeDopplerMap RangeDopplerMap.cc)

target_link_libraries(RangeDopplerMap ${MATHLIBS})

# add_unit_test(RangeDopplerMapTest.cc RangeDopplerMap)
