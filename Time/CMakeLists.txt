# -*- Mode: CMake -*-
# 
# CMake build file for the 'libTime' library
# 

# Production specification for libUnitTest
#
add_tested_library(Time SOURCES
                   TimeStamp.cc 
                   TimeStampRange.cc 
                   TimeStampRangeSet.cc
                   DEPS Logger Utils ${ACE_LIBRARY} ${QT_QTXML_LIBRARY} ${QT_QTCORE_LIBRARY}
                   TEST TimeStampTest.cc)

install(TARGETS Time LIBRARY DESTINATION lib)
