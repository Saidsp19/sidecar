# -*- Mode: CMake -*-
#
# CMake build file for the GeoFilter algorithm
#

# Production specification for the GeoFilter algorithm
#
add_algorithm(GeoFilter GeoFilter.cc)

target_link_libraries(GeoFilter)
