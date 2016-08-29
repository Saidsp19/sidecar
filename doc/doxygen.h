#ifndef SIDECAR_DOXYGEN_H	// -*- C++ -*-
#define SIDECAR_DOXYGEN_H

// NOTE: this file is not included anywhere. It serves to document the namespaces found in the reset of the
// system. It is only read by the Doxygen program. It is, however, proper C++ code.
//

/** \mainpage Radar SideCar Project

    \section intro Introduction

    The main goal of the Radar SideCar software project is to create a highly adaptable environment for running
    signal processing algorithms. Users configure signal processing streams using XML configuration files that
    define how information flows from algorithm to algorithm and how data flows from machine to machine in the
    SideCar LAN. Documentation for the existing algorithms, as well as the algorithm API may be found in the
    SideCar::Algorithms namespace.

    There are several applications available which provide a graphical interface to the user. Clicking on their
    name will lead you to their documentation.

    - SideCar::GUI::AScope a software oscilloscope for viewing data samples.

    - SideCar::GUI::Master a process manager application that shows the active processing streams running in the
    SideCar LAN.

    - SideCar::GUI::Playback an application that can playback recorded data files. Supports playback rate
    changing and looping when it reaches the end of a data file.

    - SideCar::GUI::PPIDisplay an simulation of a phosphor plot position indicator (PPI) display. Even has a
    crude simulated decay effect for those that can't live without phosphor decay.

    \section topics Topic Overviews

    The following links are to short discussions focused on a particular area of the SideCar system. Additional
    details may be found in the class documentation that they reference. Most of the topics have their own C++
    namespace associated with them, and additional information may be found by accessing the namespace entity.

    - \subpage building "Building and Installing"
    - \subpage algorithms "About Algorithms"
    - \subpage gui "GUI Applications"
    - \subpage io "I/O Overview"
    - \subpage logger "Logging Services"
    - \subpage messages "Message Formats"
    - \subpage parameter "Runtime Parameters"
    - \subpage runner "Algorithm Runners"
    - \subpage scripts "Shell Scripts Overview"
    - \subpage time "Time Processing"
    - \subpage unittest "Unit Testing Infrastructure"
    - \subpage utils "Utility Classes and Functions"
    - \subpage zeroconf "Zeroconf Interfacing"
    - \subpage xmlrpc "The XmlRpc++ Library"
*/
    
/** \page building Building and Installing

    \section prelim Preliminaries

    Adjust your command-line shell's environment so that the following variables exist and hold valid values:

    - ACE_ROOT (default /opt/sidecar/deps/ACE_wrappers)
    - BOOST_ROOT (default /opt/sidecar/deps/boost)
    - PYTHON_ROOT (default /opt/sidecar)
    - QTDIR (default /opt/sidecar/deps/qt)
    - VSIPL_ROOT (default /opt/sidecar/deps/vsipl)

    There is an env-init.sh file in this directory that may be used by Bash shell users to properly set these
    values. To use, just add the following line to your $HOME/.profile file:

    \code
    . $HOME/sidecar/trunk/env-init.sh
    \endcode

    This assumes that your SideCar source code repository is in your home directory; if it is elsewhere, then
    adjust the above path accordingly. Make sure that the above environment variables are properly set before
    continuing.

    \section External Packages

    Currently the SideCar system is using the Boost C++ templates and libraries, the ACE component of the
    ACE/TAO CORBA implementation, Trolltech's Qt graphics library, and Apples's Bonjour library (Zeroconf), all
    of which require user configuration prior to building. The external/config directory holds these
    configurations in a architecture- and host-specific manner. Each architecture name (returned by `uname') has
    a directory under external/config (eg. external/config/Linux) that contains configuration files that should
    apply to all machines of that architecture type. Within the architecture directory may be a host-specific
    directory with the name returned by `hostname' which contains configuration files used by that particular
    host.

    \subsection install_ace ACE

    Visit the <a href="http://www.cs.wustl.edu/~schmidt/ACE.html">ACE/TAO homepage</a> to obtain the source
    files to compile. Obtain at least version 5.5. Download and unzip the source into the parent directory of
    $ACE_ROOT (eg. /opt/sidecar); unzipping the source will create the ACE_wrappers directory. You need to
    create two files need in ACE_wrappers before compiling. Either create a new one or copy from
    external/config.

    - Make sure that you set the ACE_ROOT variable before continuing. From your shell prompt, verify its value
    like so:

    \code
    echo $ACE_ROOT
    \endcode
  
    If you don't see what you expect (eg. \c /opt/sidecar/ACE_wrappers) then fix before moving on.to the next
    step.

    - Move into the \c ACE_wrappers directory:

    \code
    cd $ACE_ROOT
    \endcode

    - Create an \c ace/config.h file that has two lines:

    \code  
    #define ACE_HAS_IPV6
    #include "ace/config-XXX.h"
    \endcode

    where \c XXX indicates your platform (eg. \c macosx-tiger or \c linux).

    - Create an \c include/makeinclude/platform_macros.GNU that has the following lines:

    \code
    include $(ACE_ROOT)/include/makeinclude/platform_XXX.GNU
    debug=0
    optimize=1
    \endcode

    where \c XXX again indicates your platform (eg. \c macosx_tiger or \c linux). You can change the values
    for \c debug and \c optimize if you want, but note that the ACE framework only produces one library; you
    cannot have both a debug and an optimized version of the ACE library available at the same time, only one
    or the other.

    - Move into the \c ace directory, and type

    \code
    make
    \endcode

    to compile the ACE component of ACE/TAO. You are free to issue \c make from the \c ACE_wrappers directory
    instead, but the build time will be much longer, with no benefit for the SideCar infrastructure.

    If all goes well, you should end up with a libACE library in the \c $ACE_ROOT/lib directory. Additional
    information may be found in ACE-INSTALL.html.

    \subsection install_boost BOOST C++ Framework

    Download and unzip the Boost library. The Boost install will place files under \c $BOOST_ROOT, but the
    source may be installed anywhere. Change into the top-level boost source directory and type

    \code
    bjam --layout=system --prefix=/opt/sidecar/deps/boost_1_34_1
    \endcode

    If you are on a multiprocessor system, add \c -j4 or \c -j8 before the \c --layout flag (\c --layout will
    compile the libraries without any versioning information attached. If all goes well, install the result
    with:

    \code
    bjam --layout=system --prefix=/opt/sidecar/deps/boost_1_34_1 install
    ln -s /opt/sidecar/deps/boost_1_34_1 /opt/sidecar/deps/boost
    \endcode

    \subsection install_qt QT Graphics Library

    Download and unzip the latest Qt package (4.2.3) found on the <a
    href="http://www.trolltech.com">Trolltech</a> site. Note that there are different versions for different
    operating systems; choose appropriately. Configure and build with the following commands:

    \code
    ./configure -prefix /opt/sidecar/deps/qt-4.2.3 -no-qt3support -no-exceptions -no-glib
    make
    make install
    ln -s /opt/sidecar/deps/qt-4.2.3 /opt/sidecar/deps/qt
    \endcode

    \subsection install_zeroconf Bonjour (Zeroconf) Installation

    Apple provides a free library that lets non-Apple systems participate in the Bonjour/Zeroconfig standard for
    publishing and subscribing to DNS information. It allows the SideCar applications to publish their
    connection information, and to obtain published information, removing the need to keep that information
    around in a global configuration file . Apple's <a
    hreef="http://developer.apple.com/opensource/internet/bonjour.html">Bonjour</a> web site contains a download
    link that works for many Posix compliant systems.

    To build, gunzip and untar the mDNSResponder download from that site. In unzipped directory, there are
    various subdirectiries that apply to certain operating systems. For Linux boxen, the appropriate one is
    mDNSPosix. Go there and type

    \code
    /etc/rc.d/init.d/mdns stop
    make os=linux
    \endcode

    If the build is OK, then to install type (as root)

    \code
    make install os=linux
    cp ../mDNSShared/dns_sd.h /usr/include
    /etc/rc.d/init.d/mdns start
    \endcode

    NOTE: on RedHat systems, monitor closely the results of the install, and reinstall if necessary.

    To test, try using the dns-sd application. In one term window, type in

    \code
    dns-sd -R foobar _sidecar._tcp local 123123
    \endcode

    and in another window type

    \code
    dns-sd -B _sidecar._tcp local
    \endcode

    and you should see a line for \c foobar

    \subsection install_vsipl VSIPL++ Library

    Download the latest source version (1.3) from the <a
    href="http://www.codesourcery.com/vsiplplusplus">VSIPL++</a>page of Code Sourcery's web site. Be sure to
    pick the source version. Uncompress and untar the downloaded file. Unlike the other libraries, the VSIPL++
    build/install is not the same for Linux and MacOS X, due mainly to the way VSIPL++ locates an appropriate
    LAPACK/CBLAS library.

    \subsubsection install_vsipl_linux Linux Install

    Configure VSIPL with the following command:

    \code
    ./configure --prefix=/opt/vsipl/deps/vsipl-1.3 --disable-mpi
    \endcode

    If running on a 64-bit architecture, start the above line with

    \code
    CPPFLAGS=-fPIC
    \endcode

    so that the generated libraries may be repositioned in the larger memory
    space.

    If all goes well, follow the configuration with a build and install using

    \code
    make
    sudo make install
    ln -s /opt/sidecar/deps/vsipl-1.3 /opt/sidecar/deps/vsipl
    \endcode

    and you are done.

    \subsubsection install_vsipl_macosx Mac OS X (Tiger) Install

    For Mac OS X, use the following configure command in order to use Apple's optimized LAPACK and BLAS
    libraries:

    \code
    OPTLIB=/System/Library/Frameworks/Accelerate.framework/Frameworks/vecLib.framework/Versions/Current
    CPPFLAGS=-I${OPTLIB} LDFLAGS=-L${OPTLIB} ./configure		\
    --prefix=/opt/vsipl							\
    --disable-mpi --disable-fft-long-double --enable-simd-loop-fusion	\
    --with-lapack=generic
    \endcode

    If all goes well, follow the configuration with a build and install using

    \code
    make
    sudo make install
    \endcode

    However, the resulting archive libraries from the install are unusable by the MacOS X linker (don't know
    why). To fix, you must run ranlib on them, like so:

    \code
    for LIB in ${VSIPL_ROOT}/lib/*.a; do ranlib -s ${LIB}; done
    \endcode

    \subsubsection Installation Patching

    The final stage of the installation requires patching some of the include files so that they are proper C++
    files. The following will apply the patch found in the external directory:

    \code
    cd ${VSIPL_ROOT}
    sudo patch -p0 < external/vsipl++-1.2.diff
    \endcode

    Finally, make sure the \c VSIPL_ROOT environment variable is set to the \c --prefix configuration value
    above (/opt/vsipl) in your .profile or .cshrc file.

    \subsection install_python Python 2.5 Install

    <a href="http://www.python.org.">Download</a> Python 2.5.

    \code
    ./configure --prefix=/opt/sidecar
    make
    make install
    \endcode
*/

/** Namespace for all entities specific to the 331 SideCar project.
 */
namespace SideCar {}

#endif
