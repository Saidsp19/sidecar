#!/bin/bash
#
# Helper script for CMake to complete the Install of a SideCar build.
# 

DIR="${1}"

set -x

cd "${DIR}" || exit 1

cd .. || exit 1
rm -f latest
ln -s ${DIR} latest || exit 1

exit 0
