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
add_library(Utils SHARED
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
            Wrapper.cc)

set_target_properties(Utils PROPERTIES VERSION ${SIDECAR_VERSION} SOVERSION ${SIDECAR_VERSION})

# Linkage dependencies for libUtils
#
target_link_libraries(Utils Logger ${ACE_LIBRARY})
install(TARGETS Exception Utils LIBRARY DESTINATION lib)

# Unit tests for libUtils classes
#
add_unit_test(FilePathTest.cc Utils)
add_unit_test(FileWatcherTest.cc Utils)
add_unit_test(FormatTests.cc Utils)
add_unit_test(MD5Tests.cc Utils)
add_unit_test(PoolTest.cc Utils)
add_unit_test(PowerOf2Test.cc Utils)
add_unit_test(QuickSelectTest.cc Utils)
add_unit_test(QuickSortTest.cc Utils)
add_unit_test(RingBufferTest.cc Utils)
add_unit_test(RunningAverageTest.cc Utils)
add_unit_test(RunningMedianTest.cc Utils)
add_unit_test(SineCosineLUTTest.cc Utils)
add_unit_test(WrapperTest.cc Utils)
