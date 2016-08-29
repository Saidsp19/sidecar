# -*- Mode: CMake -*-
#
# Attempt to find the VSIPL++ installation.
#

find_path(VSIPL_INCLUDE_DIR vsip PATH_SUFFIXES include HINTS /usr/local/include/vsip PATHS /usr/local /usr)
find_library(VSIPL_LIB ovxx PATH_SUFFIXES lib HINTS /usr/local/lib PATHS /usr/local /usr)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VSIPL DEFAULT_MSG VSIPL_INCLUDE_DIR VSIPL_LIB)

if(VSIPL_FOUND)
    set(VSIPL_INCLUDE_DIRS ${VSIPL_INCLUDE_DIR})
    set(VSIPL_LIBRARIES ${VSIPL_LIB})
else(VSIPL_FOUND)
    set(VSIPL_INCLUDE_DIRS)
    set(VSIPL_LIBRARIES)
endif(VSIPL_FOUND)

mark_as_advanced(VSIPL_INCLUDE_DIR VSIPL_LIB)
