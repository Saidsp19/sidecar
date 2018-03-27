# -*- Mode: CMake -*-
#
# CMake build file for the Parameter library
#

# Production specification for libParameter
#
add_tested_library(Parameter SOURCES Parameter.cc DEPS XMLRPC Utils ${BOOST_LIBRARIES}
                   TEST ParameterTest.cc TEST MonitorTests.cc)

install(TARGETS Parameter LIBRARY DESTINATION lib)
