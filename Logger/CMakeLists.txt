# -*- Mode: CMake -*-
# 
# CMake build file for the 'libLogger' library
#

# Production specification for libLogger
#
add_tested_library(Logger
                   SOURCES
                   ClockSource.cc
                   Configurator.cc
                   ConfiguratorFile.cc
                   FileWriter.cc
                   Formatters.cc
                   Log.cc 
                   LogStreamBuf.cc
                   Priority.cc
                   RemoteSyslogWriter.cc
                   RollingWriter.cc
                   StreamWriter.cc
                   SyslogWriter.cc
                   DEPS
                   Threading
                   TEST LogTests.cc)

install(TARGETS Logger LIBRARY DESTINATION lib)
