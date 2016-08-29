# -*- Mode: CMake -*-
#
# CMake build file for the MatchedFilter algorithm
#

# Production specification for the MatchedFilter algorithm
#
add_algorithm(MatchedFilter
			  MatchedFilter2.cc
			  WorkerThreads.cc
			  WorkRequest.cc
			  WorkRequestQueue.cc)

message(STATUS "FFTW3_LIBRARIES: ${FFTW3_LIBRARIES}")

target_link_libraries(MatchedFilter ${MATHLIBS})

add_unit_test(MatchedFilterTest.cc MatchedFilter)
