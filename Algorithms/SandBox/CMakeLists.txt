# -*- Mode: CMake -*-
#
# CMake build file for the SandBox algorithm
#

# Production specification for the SandBox algorithm
#
add_algorithm(SandBox SandBox.cc)

target_link_libraries(SandBox)

# add_unit_test(SandBoxTest.cc SandBox)
