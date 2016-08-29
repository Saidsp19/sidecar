# -*- Mode: CMake -*-
# 
# CMake build file for the 'libLogger' library
#

# Production specification for libLogger
#
add_library(Logger SHARED 
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
            SyslogWriter.cc)

set_target_properties(Logger PROPERTIES VERSION ${SIDECAR_VERSION} SOVERSION ${SIDECAR_VERSION})

# Linkage dependencies for libLogger
#
target_link_libraries(Logger Threading)

install(TARGETS Logger LIBRARY DESTINATION lib)

# Unit test for libLogger
#
add_unit_test(LogTests.cc Logger)
