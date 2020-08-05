# Docker Container

The `Dockerfile` contains a container definition suitable for building the SideCar software on a CentOS 8
platform.

## Building Container

To create a new Docker image, simply run `./build` in this directory. If all goes well, you should have
a `sidecar` container.

## Running Container

There are two scripts in this directory:

* `start` -- start the Docker container and provide a shell connection to it. This may be run multiple
times to obtain additional shell connections to the container.
* `stop` -- stop the Docker container. Note that this also happens when the *first* connection ends.

# Building SideCar Source

Once connected via `start`, you should then be able to build the SideCar system using the `build`
command. The source is found in $HOME/src while the build products will be in $HOME/build.
