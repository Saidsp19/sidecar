# -*- Mode: CMake -*-
#
# CMake build file for the Parameter library
#

# Production specification for libParameter
#
add_library(Parameter SHARED Parameter.cc)

set_target_properties(Parameter PROPERTIES VERSION ${SIDECAR_VERSION} SOVERSION ${SIDECAR_VERSION})

# Linkage dependencies for libUnitTest
#
target_link_libraries(Parameter XMLRPC Utils ${BOOST_LIBRARIES})

install(TARGETS Parameter LIBRARY DESTINATION lib)

# Unit tests for libParameter
#
add_unit_test(ParameterTest.cc Parameter)
add_unit_test(MonitorTests.cc Parameter)
