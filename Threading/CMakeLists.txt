# -*- Mode: CMake -*-
# 
# CMake build file for the 'libThreading' library
#

# Production specification for libThreading
#
add_tested_library(Threading
                   SOURCES Threading.cc
                   DEPS Exception ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
                   TEST ThreadingTests.cc)

install(TARGETS Threading LIBRARY DESTINATION lib)
