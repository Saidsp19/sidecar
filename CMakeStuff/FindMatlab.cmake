# -*- Mode: CMake -*-
#
# This module looks for Matlab
#
# Defines:
#  MATLAB_INCLUDE_DIRS: include path for mex.h, engine.h
#  MATLAB_LIBRARIES:   required libraries: libmex, etc
#  MATLAB_MX_LIBRARY:  path to libmx.lib
#  MATLAB_ENG_LIBRARY: path to libeng.lib

if(APPLE)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	    # message(STATUS "32-bit")
	    set(SUFFIX "maci")
    else(CMAKE_SIZEOF_VOID_P EQUAL 4)
	    # message(STATUS "64-bit")
	    set(SUFFIX "maci64")
    endif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    if(NOT MATLAB_ROOT)
	    file(GLOB MATLAB_ROOT "/Applications/MATLAB*.app")
	    # message(STATUS "GLOB MATLAB_ROOT: ${MATLAB_ROOT}")
	    if(MATLAB_ROOT)
	        list(GET MATLAB_ROOT -1 MATLAB_ROOT)
	    endif(MATLAB_ROOT)
    endif(NOT MATLAB_ROOT)
else(APPLE)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	    # message(STATUS "32-bit")
	    set(SUFFIX "glnx86")
    else(CMAKE_SIZEOF_VOID_P EQUAL 4)
	    # message(STATUS "64-bit")
	    set(SUFFIX "glnxa64")
    endif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    if(NOT MATLAB_ROOT)
	    file(GLOB MATLAB_ROOT "/opt/MATLAB_*" "/usr/local/matlab")
	    # message(STATUS "GLOB MATLAB_ROOT: ${MATLAB_ROOT}")
	    if(MATLAB_ROOT)
	        list(GET MATLAB_ROOT -1 MATLAB_ROOT)
	    endif(MATLAB_ROOT)
    endif(NOT MATLAB_ROOT)
endif(APPLE)

if(MATLAB_ROOT)

    # message(STATUS "suffix: ${SUFFIX}")

    find_path(MATLAB_INCLUDE_DIR engine.h
  			  PATH_SUFFIXES include
			  HINTS ${MATLAB_ROOT}/extern)

    find_library(MATLAB_ENG_LIB eng
  				 PATH_SUFFIXES ${SUFFIX}
  				 HINTS ${MATLAB_ROOT}/bin
				 NO_DEFAULT_PATH)

    find_library(MATLAB_MX_LIB mx
  			  	 PATH_SUFFIXES ${SUFFIX}
  				 HINTS ${MATLAB_ROOT}/bin
				 NO_DEFAULT_PATH)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(MATLAB DEFAULT_MSG MATLAB_INCLUDE_DIR
  								   	  MATLAB_MX_LIB MATLAB_ENG_LIB)
    if(MATLAB_FOUND)
	    set(MATLAB_LIBRARY_DIR ${MATLAB_ROOT}/bin/${SUFFIX})
	    set(MATLAB_INCLUDE_DIRS ${MATLAB_INCLUDE_DIR})
	    set(MATLAB_LIBRARIES ${MATLAB_ENG_LIB} ${MATLAB_MX_LIB})
	    message(STATUS "Found MATLAB at ${MATLAB_ROOT}")
    else(MATLAB_FOUND)
	    set(MATLAB_INCLUDE_DIRS)
	    set(MATLAB_LIBRARIES)
    endif(MATLAB_FOUND)

    mark_as_advanced(MATLAB_INCLUDE_DIR MATLAB_ENG_LIB MATLAB_MX_LIB)

endif(MATLAB_ROOT)
