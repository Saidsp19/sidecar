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

# Production specification for libMessages (uses and implementation libraries defined above)
#
add_library(Messages SHARED 
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
            )

set_target_properties(Messages PROPERTIES VERSION ${SIDECAR_VERSION} SOVERSION ${SIDECAR_VERSION})

# Linkage dependencies for libMessages
#
target_link_libraries(Messages MessagesBase IOBase
					  ${QT_QTXML_LIBRARY} ${QT_QTCORE_LIBRARY}
					  ${MESSAGES_EXTRA_LIBS})

install(TARGETS MessagesBase Messages LIBRARY DESTINATION lib)

# Unit tests for message classes
#
add_unit_test(CircularBufferTests.cc Messages)
add_unit_test(ExtractionsTests.cc IO)
add_unit_test(GUIDTest.cc Messages)
add_unit_test(LoaderRegistryTest.cc Messages)
add_unit_test(MetaTypeInfoTest.cc Messages)
add_unit_test(PRIMessageTest.cc IO)
add_unit_test(RadarConfigTest.cc IO)
add_unit_test(RawVideoTest.cc IO)
add_unit_test(TSPITests.cc IO)
