# -*- Mode: CMake -*-
#
# CMake build file for the IQ2AmplitudePhase algorithm
#

# Production specification for the IQ2AmplitudePhase algorithm
#
add_algorithm(IQ2AmplitudePhase IQ2AmplitudePhase.cc)

target_link_libraries(IQ2AmplitudePhase)
