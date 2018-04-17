# -*- Mode: CMake -*-
# 
# CMake build file for the Configuration library
#

# Production specification for libConfiguration
#
add_tested_library(Configuration
                   SOURCES Loader.cc RadarConfigFileWatcher.cc RunnerConfig.cc
                   DEPS MessagesBase IOBase Qt5::Core Qt5::Xml
                   TEST ConfigurationTests.cc)

install(TARGETS Configuration LIBRARY DESTINATION lib)
