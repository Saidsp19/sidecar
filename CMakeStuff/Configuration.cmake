# -*- Mode: CMake -*-
#
# CMake macros and settings for the SideCar project
#
set(SIDECAR "${PROJECT_SOURCE_DIR}")

# Create an installation directory based on the date when we were run. This will be used by 'make install' and
# to properly set the RPATH of the installed applications and libraries.
#
execute_process(COMMAND date "+%Y%m%d" OUTPUT_VARIABLE PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CMAKE_INSTALL_PREFIX "${SIDECAR}/builds/${PREFIX}")
message(STATUS "Installation location: ${CMAKE_INSTALL_PREFIX}")

# We require C++ 14
#
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set up CMake so that it will generate the proper RPATH values embedded in the applications, whether they exist
# in the build directory or the install directory.
#
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeStuff")
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_NAME_DIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Place libraries and executables in one location for ease of testing and running from the build directory.
#
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

# Now include target-specific macros and configury.
#
include("CMakeStuff/Algorithms.cmake")
include("CMakeStuff/Directories.cmake")
include("CMakeStuff/GUI.cmake")
include("CMakeStuff/Scripts.cmake")
include("CMakeStuff/UnitTests.cmake")

add_definitions(-DSIDECAR_VERSION="${SIDECAR_VERSION}")

# Specify versions of dependencies (if desired)
#
set(ACE_VERSION "")
set(BOOST_VERSION "1.60")
set(FFTW3_VERSION "")
set(MPI_VERSION "")
set(VSIPL_VERSION "")
set(OpenGL_VERSION "")
set(OpenGL_GL_PREFERENCE "GLVND")
set(Qt5_VERSION "5.10")

# Locate dependencies
#
find_package(ACE ${ACE_VERSION} REQUIRED)

set(Boost_USE_MULTITHREADED ON)
find_package(Boost ${BOOST_VERSION} REQUIRED
             date_time
             filesystem
             graph
             iostreams
             random
             serialization
             signals
             system
             thread)
find_package(FFTW3 ${FFTW3_VERSION} REQUIRED)
#find_package(MPI ${MPI_VERSION} REQUIRED)
find_package(VSIPL ${VSIPL_VERSION} REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenGL ${OpenGL_VERSION} REQUIRED)

if(APPLE AND EXISTS /usr/local/opt/qt5)

	# Homebrew installs Qt5 (up to at least 5.9.1) in /usr/local/qt5, ensure it can be found by CMake since it
	# is not in the default /usr/local prefix.
    #
    list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt5")
endif()

# For Qt installs in other locations, pass in a value for Qt5_DIR on the cmake command line, like:
#
# %  cmake -DQt5_DIR=/Users/howes/Qt/5.10.1/clang_64/lib/cmake/Qt5 ..
#
find_package(Qt5 COMPONENTS Core Concurrent Gui OpenGL Network PrintSupport Svg Widgets Xml REQUIRED)

message(STATUS "ACE_INCLUDE_DIR: ${ACE_INCLUDE_DIR}")
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
message(STATUS "FFTW3_INCLUDE_DIRS: ${FFTW3_INCLUDE_DIRS}")
message(STATUS "VSIPL_INCLUDE_DIRS: ${VSIPL_INCLUDE_DIRS}")

# Platform-specific settings
#
if(UNIX)

    # Unix-like systems. Only type we currently support
    #
    if(APPLE)

        # Definitions for MacOS X
        #
        add_definitions(-Ddarwin -D_XOPEN_SOURCE -D_DARWIN_C_SOURCE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
        set(CMAKE_OSX_ARCHITECTURES "x86_64")
        set(SIDECAR_PACKAGING MACOSX_BUNDLE)

    else(APPLE)

        # Definitions for Linux (and Solaris *** FIX ***)
        #
        add_definitions(-Dlinux)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -std=c++14 -fPIC")
    endif(APPLE)
endif(UNIX)

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

# link_directories($ENV{FFTW_ROOT}/lib $ENV{VSIPL_ROOT}/lib)
set(MATHLIBS lapack cblas fftw3 fftw3f ${CMAKE_THREAD_LIBS_INIT} ${MPI_CXX_LIBRARIES})

include_directories(${PROJECT_SOURCE_DIR} ${ACE_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${MPI_INCLUDE_PATH})

message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY:" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
