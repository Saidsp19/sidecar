# -*- Mode: CMake -*-
# 
# CMake build file for the 'libUtils' and 'libException' libraries
# 

# Production specification for libException
#
add_library(Exception SHARED Exception.cc)
set_target_properties(Exception PROPERTIES VERSION ${SIDECAR_VERSION} SOVERSION ${SIDECAR_VERSION})

# Production specification for libUtils
#
add_tested_library(Utils
                   SOURCES
                   AzimuthSweep.cc
                   BeamWidthFilter.cc
                   CmdLineArgs.cc
                   FileWatcher.cc
                   FilePath.cc
                   Format.cc
                   IO.cc
                   MD5.cc
                   Pool.cc
                   RingBuffer.cc
                   RunningAverage.cc
                   RunningMedian.cc
                   SineCosineLUT.cc
                   Utils.cc
                   Wrapper.cc

                   DEPS Logger ${ACE_LIBRARY}

                   TEST FilePathTest.cc
                   TEST FileWatcherTest.cc
                   TEST FormatTests.cc
                   TEST MD5Tests.cc
                   TEST PoolTest.cc
                   TEST PowerOf2Test.cc
                   TEST QuickSelectTest.cc
                   TEST QuickSortTest.cc
                   TEST RingBufferTest.cc
                   TEST RunningAverageTest.cc
                   TEST RunningMedianTest.cc
                   TEST SineCosineLUTTest.cc
                   TEST WrapperTest.cc)

install(TARGETS Exception Utils LIBRARY DESTINATION lib)
