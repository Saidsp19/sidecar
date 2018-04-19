# -*- Mode: CMake -*-
# 
# CMake build file for the 'libTime' library
# 

add_tested_library(Time SOURCES
                   TimeStamp.cc 
                   TimeStampRange.cc 
                   TimeStampRangeSet.cc
                   DEPS Logger Utils ${ACE_LIBRARY} Qt5::Core
                   TEST TimeStampTest.cc)

install(TARGETS Time LIBRARY DESTINATION lib)
