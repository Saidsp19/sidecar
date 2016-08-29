# -*- Mode: CMake -*-
#
# CMake build file for the Recorder algorithm
#

# Production specification for the Recorder algorithm
#
add_algorithm(Recorder Recorder.cc)

target_link_libraries(Recorder)

add_unit_test(RecorderTest.cc Recorder)
