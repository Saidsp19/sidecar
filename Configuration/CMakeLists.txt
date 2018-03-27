# -*- Mode: CMake -*-
# 
# CMake build file for the Configuration library
#
include_directories(${QT_INCLUDES})

# Production specification for libConfiguration
#
add_tested_library(Configuration
                   SOURCES Loader.cc RunnerConfig.cc
                   DEPS Messages ${QT_LIBRARIES}
                   TEST ConfigurationTests.cc)

install(TARGETS Configuration LIBRARY DESTINATION lib)
