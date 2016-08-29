# -*- Mode: CMake -*-
# 
# CMake build file for the Zeroconf library
# 

# Production specification for libZeroconf
#
add_library(Zeroconf SHARED 
			ACEMonitor.cc
			ASIOMonitor.cc
			Browser.cc
			Publisher.cc
			ResolvedEntry.cc
			ServiceEntry.cc
			Transaction.cc
            )

set_target_properties(Zeroconf PROPERTIES VERSION ${SIDECAR_VERSION} SOVERSION ${SIDECAR_VERSION})

# Apple does not have a libdns_sd library to link with.
#
set(DNSSD dns_sd)
if(APPLE)
    set(DNSSD "")
endif(APPLE)

# Linkage dependencies for libZeroconf
#
target_link_libraries(Zeroconf Utils ${DNSSD})

install(TARGETS Zeroconf LIBRARY DESTINATION lib)

add_unit_test(ZeroconfTests.cc Zeroconf)
add_unit_test(ASIOZeroconfTests.cc Zeroconf)
