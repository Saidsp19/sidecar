#!/bin/bash
#
# Helper script for CMake on Mac OS X to disable app nap for an executable.
# 

APP="${1}"

set -x

defaults write ${APP} NSAppSleepDisabled -bool YES

exit 0
