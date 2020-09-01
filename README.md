# Introduction

The main goal of the Radar SideCar software project is to create a highly adaptable environment for running
signal processing algorithms. Users configure signal processing streams using XML configuration files that
define how information flows from algorithm to algorithm and how data flows from machine to machine in the
SideCar LAN.

There are several applications available which provide a graphical interface to the user:

* AScope - a software oscilloscope for viewing data samples ([additional info](http://keystrokecountdown.com/articles/ascope/index.html))
* BScope - a rectangular representation of radar returns - contrast with PPIDisplay's polar display ([additional info](http://keystrokecountdown.com/articles/bscope/index.html))
* Master - a process manager application that shows the active processing streams running in the SideCar LAN
* Playback - an application that can playback recorded data files. Supports playback rate changing and looping when it reaches the end of a data file
* PPIDisplay - a simulation of a phosphor plot position indicator (PPI) display. Even has a crude simulated decay effect for those that can't live without phosphor decay ([additional info](http://keystrokecountdown.com/articles/radardisplay/index.html))

Various signal processing algorithms can be found in the Algorithms directory.

# Documentation

Many but not all of the source headers have JavaDoc-style comments. Using `doxygen`, one can convert them into
a static web site:

```
% cd ~/src/sidecar
% doxygen doc/Doxyfile
```

The HTML output will reside under the `docs/`. It is available online at
https://bradhowes.github.io/sidecar/index.html

# Building

Building has worked on various Linux and MacOS X systems. Nearly all of the code is C++ with some bits in C. The
code relies on the following external libraries:

* [ACE](https://www.dre.vanderbilt.edu/~schmidt/ACE.html) - should work with at least version 5.5
* [Boost C++ Framework](http://boost.org) - built and tested using v1.60.0
* [Qt](http://qt.io) - built and tested using v5.10.1
* [Zeroconf](http://www.zeroconf.org) - on Linux, requires Avahi's mDNS API emulation support (legacy)
* [OpenVSIP](https://github.com/bradhowes/openvsip) - vector signal and image processing library

[CMake](https://cmake.org) scripts control the building process, and require at least v3.6. Hopefully it will
find everything it needs and run without errors. If not, all configury is done in
`CMakeStuff/Configuration.cmake`. Locate the step that failed and try to figure out why it failed. Usually a
path will be wrong or a package is missing. Check out the [CMake
documentation](https://cmake.org/cmake/help/v3.17/command/find_package.html?highlight=find_package) for the
`find_package` command for additional details and hints on how to make it work.

The usual way to build is to create a build directory, move into it and then execute `cmake ..` to generate
Makefiles, like so:

```
% cd ~/src/sidecar
% mkdir build
% cd build
% cmake ..
```

If successful, one should then just type `make` to build the executables.

> **NOTE:** the SideCar system consists of a collection of shared libraries, programs, and algorithms (packaged
    as dynamically loadable modules). As each one is built, unit and integration tests are automatically run as
    part of the built. Hopefully these tests will all succeed, but if not they should help identify what is
    causing the issue.

> **NOTE:** if the build fails because of a `ZeroconfTests` failure, you probably need to enable multicast DNS on
    your system and perhaps modify your firewall configuration for multcast traffic (or just disable the firewall).

## Linux Fedora 27 Installation Notes

Originally, I developed the software on both macOS and openSUSE. For kicks, I tried and succeeded getting most of
the code and apps built on a Fedora 27 installation. This was a bit more involved than the macOS install
described below, but it does work. The following packages are needed on the system:

* ace (6.5.11) (see [https://download.opensuse.org/repositories/devel:/libraries:/ACE:/micro/](https://download.opensuse.org/repositories/devel:/libraries:/ACE:/micro/) for RPMs)
* ace-devel
* autoconf
* avahi-compat-libdns_sd (0.7-11)
* avahi-compat-libdns_sd-devel (0.7-11)
* boost (1.73)
* boost-devel
* cmake (3.18.2)
* fftw (3.3.8)
* fftw-devel
* freeglut-devel (3.0.0-6)
* gcc / clang
* lapack-devel (3.8.0-7)
* make
* mesa-demos (8.3.0-9)
* qt5 (5.15.0)
* qt5-qtsvg
* qt5-qtsvg-devel
* qt5-qttools-devel

Other versions would probably work as well, but these are what I have most recently used and tested.

The software also needs an implementation of the
[Vector Signal and Image Processing Library](http://openvsip.org) spec. I have a slightly modified fork of
[OpenVSIP](https://github.com/bradhowes/openvsip) that compiles on macOS. To install:

```
% git clone https://github.com/bradhowes/openvsip.git
% cd openvsip
% ./autogen.sh
% mkdir objdir
% cd objdir
% ../configure --disable-exceptions # It *should* find the LAPACK install from above
```

If all goes well with `configure`, you should be able to build:

```
% make
% make install
```

If `configure` has issues, you will have to explore its options to see if you can get it to do what you want.

Now to build SideCar:

```
% git clone https://github.com/bradhowes/sidecar.git
% cd sidecar
% mkdir build
% cd build
% cmake ..
```

Hopefully `CMake` will find everything and run without errors. Next:

```
% make
```

This will take some time -- you can try adding `-j N` where N is something like the number of CPUs on your
machine. Note that currently the `spectrum` application does not build on Fedora 27 due to OpenGL configuration
issues.

> NOTE: the `ppidisplay` program will not run if the window system is Wayland (the `ascope` application works fine).
> You should be able to chose to use an X11 windowing environment from the login screen. I have no time to work on
> changes to make the Open GL applications work properly when run in Wayland.

## CentOS 8 Installation Notes

Due to an interest expressed in using the code on a CentOS 8 system, I managed to install and build the system
after some effort and a lot of Googling. The dependencies are the same as above (more or less) but I had to
locate the ACE RPMs that were not available on EPEL or PowerTools repositories. I also needed to disable the
firewall to see multicast DNS traffic (configuring would be a better option, but this was on a virtual machine
with no external network connectivity). Finally, I had to install Python (3.8) and used `sudo alternatives
--config python` to make a `python` command available. After all of that, the build and unit tests started to
work.

## Docker Container Build

For the CentOS 8 build above, I now have a `Dockerfile` in the `containers` directory that can be used to
build the source in controlled environnment. It is noticably *slower* to build this way but it does reduce
the amount of work necessary to create a Linux environment for building.

The `containers/build` script will create a new "sidecar" Docker container, provisioned with the right tools to
do a build. When complete, the `containers/start` script can be used to start up the container and obtain a
shell connection to it. Once connected, the shell has its own `build` script which compiles the sources from the
host to generate binaries in the container. In the container, the sources are in `/home/sidecar/src` and the
binaries will be in `/home/sidecar/build/bin`.

> NOTE: currently, there is no OpenGL support from the container via X11, so only the `AScope` display
> application will run properly.

## MacOS Installation Notes

Here are some brief notes on getting everything to run on macOS. This works on my MacBook Pro 2017 running
Catalina (10.15.4). Install the [Brew](https://brew.sh) package manager if you don't already have it. Next,
install the following packages (I've noted the versions that I currently have)

```
% brew install cmake # 3.18.2
% brew install ace # 6.5.11
% brew install boost # 1.73
% brew install fftw # 3.3.8
% brew install open-mpi # 4.0.3
% brew install qt # 5.15.0
```

The software also needs an implementation of the
[Vector Signal and Image Processing Library](http://openvsip.org) spec. I have a slightly modified fork of
[OpenVSIP](https://github.com/openvsip/openvsip) that compiles on macOS. To install:

```
% git clone https://github.com/bradhowes/openvsip.git
% cd openvsip
% ./autogen.sh
% mkdir objdir
% cd objdir
% ../configure --disable-exceptions --with-lapack=apple # Only if on macOS
```

If all goes well with `configure`, you should be able to build:

```
% make
% make install
```

If `configure` has issues, you will have to explore its options to see if you can get it to do what you want.

Now to build SideCar:

```
% git clone https://github.com/bradhowes/sidecar.git
% cd sidecar
% mkdir build
% cd build
% cmake ..
```

Hopefully `CMake` will find everything and run without errors. Next:

```
% make
```

This will take some time -- you can try adding `-j N` where N is something like the number of CPUs on your
machine.

> **NOTE:** there may be some rendering issues in the Qt applications if your system is running in the *dark
    mode* setting on macOS 10.15 (Catalina). I have tried to fix some, but much more time should be spent on
    fixing the various Qt bits that no longer work right or well.

## Environment Configuration

Various parts of the software like to know where it was installed, and they expect to find an environment
variable called `SIDECAR` that holds the installation directory. For our purposes, we will just set it to the
`build` directory that holds the output of the `make` operation.

```text
% export SIDECAR="$PWD"
```

We also need to save this value for any future processes to find. The best place to store it is in
`$HOME/.profile` or `$HOME/.bash_profile` (I've only tested with the first).

```text
% echo export SIDECAR="'$SIDECAR'" >> $HOME/.profile
```

## Post Install

The SideCar applications rely on IPv4 multicast for transmitting data between them (using Zeroconf/Bonjour to
determine who is around to listen to). For best results, you should probably route multicast traffic over your
loopback interface. The apps are hardcoded (I think) to use the address `237.1.2.100` for multicast traffic.

```
% sudo route add -net 237.1.2.100/32 -interface lo0
```

> NOTE: this should *not* be done if you want to share multicast data with other systems on your network. It is great
> for development on one system so you don't disrupt the network with tons of packets.

There are some binary files in the `data/pri` directory that need to be joined before they can be used:

```
% cd $SIDECAR/data/pri
% bash prijoin.sh
```

# Demonstration

If all of the above went well, we can test out the apps in the `build/bin` directory. First, let's try and emit
some data:

```
% open $SIDECAR/bin/priemitter.app
```

You should see something like

![](images/priemitter1.png)

Now, try loading the data file `$SIDECAR/data/pri/20t10scans.pri` by selecting "File > Open" (**⌘ O**) and
navigating to the `20t10scans.pri` file. Make sure that the `Connection Type` is set to `Multicast` in the main
window; it defaults to using `TCP`.

![](images/priemitter2.png)

The window shows `Connections: 0` because no one is listening for data on the `NegativeVideo` channel. Let's see
if we can get an app to do so:

```
% open $SIDECAR/bin/ascope.app
```

The AScope app will show something like an osciliscope display that plots data samples from a data source.

![](images/ascope1.png)

When it starts up, it won't be connected to anything. Bring up the `Channels Window` (**⌘ 1**) and you should
see one channel available:

![](images/channels.png)

Click on the `+` button at the bottom to subscribe to the channel and close the window. Back in the `priemitter`
app, click the `Start` button. With a good amount of luck, you should see something like this:

![](images/ascopeSignal.gif)

Now, let's try for the big fish: `ppidisplay`. This application attempts to display incoming data in a format
similar to a radar display. However, unlike its cousin `ascope`, this application needs to know a bit about the
incoming data in order to properly display it. We will point it to an XML configuration file that properly
describes the format of the `20t10scans` data we are emitting:

```
% export SIDECAR_CONFIG="$SIDECAR/data/pri/20t10scans.xml"
% open $SIDECAR/bin/ppidisplay.app
```

Bring up the `Channel Selector` window with **⌘ 1** and select `NegativeVideo` for the `Video` source. You
should start to see data flow to the display.

![](images/ppidisplaySignal.gif)

# Signal Processing Chains

There is a GUI application called `master` which lets you launch and monitor signal processing chains --
collections of algorithms that process input streams and generate new output streams. Unfortunately, getting
this to work properly on a new host is a bit of dark art. Below is a minimal attempt to document what needs to
be done to get at least a minimal processing chain up and running

## SSH Connectivity

The first thing that needs to be done is to enable SSH access to the machine(s) you wish to run the processing
chains. For demonstration purposes we will run all processing locally, so we need SSH to the local machine. This
is disabled by default on macOS and for a good reason: **enabling SSH can allow others to access your machine**.
Just be aware of that.

![](images/enablessh.png)

Next, we need to be able to access the host via SSH using a public key. Since we are trying to reach our own
machine using SSH, the easiest way to enable this is to do the following (assuming you have a public key with
the name `id_rsa.pub`)

```text
% cat $HOME/.ssh/id\_rsa.pub >> $HOME/.ssh/authorized\_keys
% ssh localhost
The authenticity of host 'localhost (::1)' can't be established.
ECDSA key fingerprint is SHA256:2Me0Xq0lP5ruqvhQQn8UkHm3NopftDviyZvJhGr7ZYg.
Are you sure you want to continue connecting (yes/no)? yes

Warning: Permanently added 'localhost' (ECDSA) to the list of known hosts.
Last login: Wed Apr 11 10:36:32 2018 from 192.168.12.132
% exit
logout
Connection to localhost closed.
%
```

(Future connections will be less noisy)

## Launching Master Application

The `master` GUI app allows one to load XML configuration files that describe signal processing chains and then
_launch_ them and monitor their performance.

![](images/master1.png)

Load the `basic` configuration file found at `$SIDECAR/data/configs/basic.xml` by pressing the big plus sign
(+) in the upper-left region corner. Now try launching it by pressing the `Launch` button. If your environment
and SSH are set up properly, you should see an alert saying all was well and your `master` view should look
something like this:

![](images/master2.png)

The `basic.xml` configuration defines just one processing stream -- "Basic Extraction" -- with four algorithms:

- NCIntegrate -- a smoothing filter
- Threshold -- a filter that converts floating values into booleans depending on their magnitude
- MofN -- further filters boolean values such that a true value depends on having a certain number of
  surrounding true values.
- Extract -- another conversion filter that generates an _extraction_ record for each true value from MofN.

The `master` view shows real runtime state from each of the algorithms. The view also has a button for each
algorithm that allows you to edit runtime parameters declared by the algorithm.

## Sending Data

Start up the `priemitter` application mentioned earlier and reload the `20t10scans.pri` file. Make sure that the
multicast address is `237.1.2.102` -- this is the same value that is in the `basic.xml`. Notice that after
loading, the `Connections` value still shows 0, unlike when we ran `ascope`. This is because Sidecar processing
streams do not do any work unless there is something that is consuming their output, and we don't have anything
yet that is doing so. However, the streams *will* perform work if they are set in the "AutoDiag" state. Click on
the "State" popup and select "AutoDiag" (or "Calibrate"). Now, the `Connections` count should change to `1` if
all the plumbing is working properly.

> NOTE: an easy way to debug issues is to launch a GUI app from a termainal shell, such as
> `$SIDECAR/bin/master.app/Contents/MacOS/master`. That way log messages from the application will appear in the
> terminal window. Also, log levels can be set for `master` in the "Logger Configuration Window". Note that
> DEBUG level will emit a lot of data and make the GUI rather slow

Press "Start" button in the `priemitter` application. You should see the real-time stats for the algorithms turn
to green and show messages per second stats:

![](images/master3.png)

## Viewing Data

The basic processing stream above consumes data over the `NegativeVideo` channel, and it creates three new data
streams:

- NCIntegrate -- the smoothed output from the NCIntegrate algorithm
- Threshold -- a stream of binary values from thresholding the smoothed values
- MofN -- a stream of binary values from the M-of-N filtering
- Extractions -- a stream of _extraction_ records

The `ascope` application can only show streams of floating-point data. However, the `ppidisplay` app
demonstrated above can show all three data types (floating-point, binary, and extractions). Simply show the
"Channels Window" (**⌘ 1**) and you should see the additional available channels. By default binary data appears
as dark blue (configurable).

![](images/ppidisplayTracks.gif)
