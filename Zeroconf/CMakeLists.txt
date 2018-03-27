# -*- Mode: CMake -*-
# 
# CMake build file for the Zeroconf library
# 

# Production specification for libZeroconf
#
set(DNSSD dns_sd)
if(APPLE)
    set(DNSSD "")
endif(APPLE)

add_tested_library(Zeroconf
                   SOURCES
			       ACEMonitor.cc
			       ASIOMonitor.cc
			       Browser.cc
			       Publisher.cc
			       ResolvedEntry.cc
			       ServiceEntry.cc
			       Transaction.cc
                   DEPS Utils ${DNSSD}
                   TEST ZeroconfTests.cc
                   TEST ASIOZeroconfTests.cc
                   )

install(TARGETS Zeroconf LIBRARY DESTINATION lib)
