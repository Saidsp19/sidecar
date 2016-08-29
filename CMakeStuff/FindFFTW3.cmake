# -*- Mode: CMake -*-
#
# Attempt to find the FFTW3 installation.
#

find_path(FFTW3_INCLUDE_DIR fftw3.h PATH_SUFFIXES include HINTS ${SIDECAR_DEPS}/fftw PATHS /usr/local /usr)
find_library(FFTW3_LIB1 fftw3 PATH_SUFFIXES lib64 lib HINTS ${SIDECAR_DEPS}/fftw PATHS /usr/local /usr)
find_library(FFTW3_LIB2 fftw3f PATH_SUFFIXES lib64 lib HINTS ${SIDECAR_DEPS}/fftw PATHS /usr/local /usr)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFTW3 DEFAULT_MSG FFTW3_INCLUDE_DIR FFTW3_LIB1 FFTW3_LIB2)

if(FFTW3_FOUND)
    set(FFTW3_INCLUDE_DIRS ${FFTW3_INCLUDE_DIR})
    set(FFTW3_LIBRARIES ${FFTW3_LIB1} ${FFTW3_LIB2})
else(FFTW3_FOUND)
    set(FFTW3_INCLUDE_DIRS)
    set(FFTW3_LIBRARIES)
endif(FFTW3_FOUND)

mark_as_advanced(FFTW3_INCLUDE_DIR FFTW3_LIB1 FFTW3_LIB2)
