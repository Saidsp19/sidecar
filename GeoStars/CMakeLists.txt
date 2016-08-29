# -*- Mode: CMake -*-
# 
# CMake build file for the 'libGeeStars' library.
# 

add_library(GeoStars SHARED geoPoint.c geoEllips.c geoMag.c)

set_target_properties(GeoStars PROPERTIES
					  VERSION ${SIDECAR_VERSION}
					  SOVERSION ${SIDECAR_VERSION})

install(TARGETS GeoStars LIBRARY DESTINATION lib)
