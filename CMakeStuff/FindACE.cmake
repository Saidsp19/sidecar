# -*- Mode: CMake -*-
#
# Attempt to find the ACE installation.
#

find_path(ACE_INCLUDE_DIR ace)
find_library(ACE_LIBRARY ACE PATH_SUFFIXES lib lib64)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ACE DEFAULT_MSG ACE_INCLUDE_DIR ACE_LIBRARY)

mark_as_advanced(ACE_INCLUDE_DIR ACE_LIBRARY)
