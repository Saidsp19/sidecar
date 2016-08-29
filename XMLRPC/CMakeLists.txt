# -*- Mode: CMake -*-
# 
# CMake build file for the 'libXMLRPC' library
# 

# Production specification for libXMLRPC
#
add_library(XMLRPC SHARED 
			XmlRpcClient.cpp
			XmlRpcDispatch.cpp
			XmlRpcServer.cpp
			XmlRpcServerConnection.cpp
			XmlRpcServerMethod.cpp
			XmlRpcSocket.cpp
			XmlRpcSource.cpp
			XmlRpcUtil.cpp
			XmlRpcValue.cpp
            )

set_target_properties(XMLRPC PROPERTIES VERSION ${SIDECAR_VERSION} SOVERSION ${SIDECAR_VERSION})

install(TARGETS XMLRPC LIBRARY DESTINATION lib)
