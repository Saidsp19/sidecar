# -*- Mode: CMake -*-
# 
# CMake build file for the 'libUnitTest' library
#

# Production specification for libUnitTest
#
add_library(UnitTest SHARED RunResults.cc Suite.cc TestObj.cc)
set_target_properties(UnitTest PROPERTIES VERSION ${SIDECAR_VERSION} SOVERSION ${SIDECAR_VERSION})

# Linkage dependencies for libUnitTest
#
target_link_libraries(UnitTest Logger_tests)
install(TARGETS UnitTest LIBRARY DESTINATION lib)

# Unit test for libUnitTest
#
add_unit_test(TestTest.cc)
