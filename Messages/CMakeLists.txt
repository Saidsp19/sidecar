# -*- Mode: CMake -*-
# 
# CMake build file for the 'libMessages' library
#

# Production specification for libMessages (uses and implementation libraries defined above)
#
add_library(MessagesBase SHARED 
			Attributes.cc
			CircularBuffer.cc
			GUID.cc
			Header.cc
			LoaderRegistry.cc
			MetaTypeInfo.cc
			RadarConfig.cc
			RadarConfigFileWatcher.cc
			VMEHeader.cc
			XmlStreamReader.cc
            )

set_target_properties(MessagesBase PROPERTIES VERSION ${SIDECAR_VERSION} SOVERSION ${SIDECAR_VERSION})

# Linkage dependencies for libMessages
#
target_link_libraries(MessagesBase GeoStars Time XMLRPC
					  ${QT_QTXML_LIBRARY}
					  ${QT_QTCORE_LIBRARY})

# Production specification for libMessages (uses libraries defined above)
#
add_tested_library(Messages
                   SOURCES
			       BinaryVideo.cc
			       BugPlot.cc
			       Complex.cc
			       Extraction.cc
			       PRIMessage.cc
			       RawVideo.cc
			       Segments.cc
			       Track.cc
			       TSPI.cc
			       Video.cc
			       ${MESSAGES_EXTRA_SRCS}
                   DEPS MessagesBase IOBase ${QT_QTXML_LIBRARY} ${QT_QTCORE_LIBRARY} ${MESSAGES_EXTRA_LIBS}
                   TEST CircularBufferTests.cc
                   TEST ExtractionsTests.cc
                   TEST GUIDTest.cc
                   TEST LoaderRegistryTest.cc
                   TEST MetaTypeInfoTest.cc
                   TEST PRIMessageTest.cc
                   TEST RadarConfigTest.cc
                   TEST RawVideoTest.cc
                   TEST TSPITests.cc
            )

install(TARGETS MessagesBase Messages LIBRARY DESTINATION lib)
