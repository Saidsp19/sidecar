# -*- Mode: CMake -*-
# 
# CMake build file for the 'libThreading' library
#

# Production specification for libThreading
#
add_library(Threading SHARED Threading.cc)
set_target_properties(Threading PROPERTIES VERSION ${SIDECAR_VERSION} SOVERSION ${SIDECAR_VERSION})

# Linkage dependencies for libThreading
#
target_link_libraries(Threading Exception ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS Threading LIBRARY DESTINATION lib)

# Unit test for libThreading
#
add_unit_test(ThreadingTests.cc Threading)
