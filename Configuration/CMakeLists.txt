# -*- Mode: CMake -*-
# 
# CMake build file for the Configuration library
#
include_directories(${QT_INCLUDES})

# Production specification for libConfiguration
#
add_library(Configuration SHARED Loader.cc RunnerConfig.cc)

set_target_properties(Configuration PROPERTIES VERSION ${SIDECAR_VERSION} SOVERSION ${SIDECAR_VERSION})

# Linkage dependencies for libConfiguration
#
target_link_libraries(Configuration Messages ${QT_LIBRARIES})

install(TARGETS Configuration LIBRARY DESTINATION lib)

# Unit test for libConfiguration
#
add_unit_test(ConfigurationTests.cc Configuration)
