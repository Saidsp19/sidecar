#ifndef SIDECAR_SCRIPTS_DOXYGEN_H // -*- C++ -*-
#define SIDECAR_SCRIPTS_DOXYGEN_H

/** \page scripts Scripts Directory

    Although the SideCar infrastructure is written in C++, some of the utility programs exist as Bash shell
    scripts and Python scripts. Some of the utilities are for developers, while others are meant for SideCar
    end-users. The list below gives the script command as it would be found in the \c Scripts directory; the
    command name for the executable based on the source file is the filename without the suffix.

    - azsim.sh Enable or disable the azimuth simulator available on the SideCar VME system. Relies on the \c
    expect utility to do the communicating with the VxWorks runtime. Since the only portal to the VME system
    exists on the data-recorder server, this script uses ssh to establish a connection to data-recorder and to
    run the appropriate \c expect script. The expect scripts \c azsimon.ex and \c azsimoff.ex exist in the \c
    data directory.

    - install-build.sh Installs the contents of the \c bin and \c lib directories under the user's SideCar
    development directory. Creates a new installation directory in \c ${SIDECAR}/builds directory using the
    date when he script was run (YYMMDD), adding a numerical suffix if necessary to make the directory unique.
    Creates a symbolic link called \c ${SIDECAR}/builds/latest to the newly created directory. Also creates if
    not present, a link from \c ${SIDECAR}/builds/latest/data to \c ${SIDECAR}/data.

    \rem NOTE: the option \c -r will cause \b install-build to \b replace an existing directory with the same
    YYMMDD value.

    - install-gui.sh Creates RedHat desktop entries for the SideCar GUI applications as well as some LLDRFM
    commands. A top-level SideCar menu becomes available under the "red hat" main menu. Within that menu are
    submenus for GKrellm machine monitors, LLDRFM commands, and buttons to start SideCar GUI commands
    (SideCar::GUI::AScope, SideCar::GUI::Master, SideCar::GUI::Playback, SideCar::GUI::PPIDisplay,
    SideCar::GUI::PRIEmitter)

    - monitor.sh Startup script for SideCar machine monitors that use the GKrellm application. Copies to the
    user's home directory configuration files from the ${SIDECAR}/data directory to give a common
    look-and-feel for all of the machines and users.

    - newalg.sh Developer utility that will create a new algorithm directory inside of the Algorithms top-level
    directory and populate it with template files. The user provides a name for the new algorithm, and the
    script manipulates the template files to reflect the new algorithm name. Adds the algorithm directory to
    the Algorithms build infrastructure. Should result in a compilable and usable algorithm. However, the
    developer must edit the resulting C++ files to get them to perform the new algorithm.

    - recctl.py Python script that controls the recording process of a Master application running on the local
    machine. Takes one argument, which is either \c start or \c stop. Echos back the result from the Master
    application. For the \c start command, this will contain the new recording directory if Master was able to
    start a new recording. Currently, this script is used by the Matlab LLDRFM calibration routines.

    - scdiffs.sh Developer utility that reports back the differences between two source trees. The location of
    the two trees is fixed, corresponding to how the supreme developer, Brad Howes, likes to have things set
    up. The baseline source tree is expected in ${HOME}/trunk, and the changed source tree is
    ${HOME}/sidecar/trunk. These locations also exist in the scunpack.sh script, described below.

    - scpush.sh Developer utility that identifies changes made in the developer's local sandbox, and transfers
    the changed files to the developer's account on blade-4. This is used by Brad Howes to migrate chanages
    from his machine, \c harrison, to \c blade-4. As such, it is probably too esoteric to be useful for
    others.

    - sctarup.sh Developer utility that creates a compressed tar file containing the source code from a
    developer's sandbox. Although it should run on any system, it is meant to run on a MacOS X system, as it
    updates the names for special MacOS X burn folders so that they contain the Subversion release number of
    the sources being burned to CD-ROM.

    - scunpack.sh Developer utility that unpacks a compressed tar file created by the sctarup.sh script above.
    Performs the extraction twice, first into ${HOME}/trunk and then into ${HOME}/sidecar/trunk. As explained
    in the description of the scdiffs.sh script, the first location is considered a read-only copy of the
    source tree, and is used as a baseline for scdiffs.sh to determine what if any changes have been made the
    the code in the second location. If not running on \c blade-4, copies the tar file to \c blade-4, so that
    the developer may run scunpack there as well.

    - startup.sh Shell script that sets up the run environment prior to starting a SideCar executable. First, it
    sources in the contents of the ${SIDEAR}/sidecar.sh file, which should set the other environment variables
    the SideCar system requires (ACE_ROOT, BOOST_ROOT, etc.) Next, it tries to create a log file to use for
    the application that will contains output from the applications standard output and standard error text
    streams. Finally, it invokes the application. All of the RedHat desktop items created by the
    install-gui.sh script use this script to execute the SideCar GUI applications. Also, SideCar::GUI::Master
    uses startup.sh when it creates remote runner applications. As such, extreme care must be taken prior to
    installing a new version of this file; a slight error can make the SideCar framework unusable.
*/

#endif
