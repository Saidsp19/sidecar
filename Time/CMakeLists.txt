# -*- Mode: CMake -*-
# 
# CMake build file for the 'libTime' library
# 

# Production specification for libUnitTest
#
add_library(Time SHARED 
            TimeStamp.cc 
            TimeStampRange.cc 
            TimeStampRangeSet.cc)

set_target_properties(Time PROPERTIES VERSION ${SIDECAR_VERSION} SOVERSION ${SIDECAR_VERSION})

# Linkage dependencies for libTime
#
target_link_libraries(Time Logger Utils ${ACE_LIBRARY} ${QT_QTXML_LIBRARY} ${QT_QTCORE_LIBRARY})
install(TARGETS Time LIBRARY DESTINATION lib)

# Unit tests for libTime
#
add_unit_test(TimeStampTest.cc Time)
