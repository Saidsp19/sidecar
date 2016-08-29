# -*- Mode: CMake -*-
# 
# CMake build file for the coherent integrate algorithm
# 

add_algorithm(CIntegrate CIntegrate.cc)

target_link_libraries(CIntegrate ${MATHLIBS})

add_unit_test(CIntegrateTest.cc CIntegrate)
