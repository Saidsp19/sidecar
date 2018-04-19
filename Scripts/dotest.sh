#!/bin/bash

# Run a unit test, recording any output to a temporary file. If the test fails, dump out the
# temporary file. Arguments:
#
# - EXE -- executable to run
# - LOG -- file to use for log messages
# - OK  -- file to create if the test passes
# - DIR -- library to prepend to [DY]LD_LIBRARY_PATH that contains libraries of the current build
#

# Run test, redirecting stdout, stderr, and stdlog to a temporary file.
#
rm -f "${2}" "${3}"

# !!! Hack !!!
#
# For the MatlabBridge component, we need to have some MATLAB libraries in our path. However, we
# don't want to polute our LIBRARY_PATH env variable with it all the time because the MATLAB
# directory contains some stale versions of libraries we do use. The DYLD_... variable is the name
# used on Mac OS X.
#
# if [[ -n "${MATLAB_ROOT_LIB}" ]]; then
#     LD_LIBRARY_PATH="${MATLAB_ROOT_LIB}:${LD_LIBRARY_PATH}"
#     DYLD_LIBRARY_PATH="${MATLAB_ROOT_LIB}:${DYLD_LIBRARY_PATH}"
# fi

# Update paths to use to current build lib directory
#
export LD_LIBRARY_PATH="${4}:${LD_LIBRARY_PATH}"
export DYLD_LIBRARY_PATH="${4}:${DYLD_LIBRARY_PATH}"

# Record environment and commnand that will run
#
echo "LD_LIBRARY_PATH: ${LD_LIBRARY_PATH}" 1>> "${2}"
echo "DYLD_LIBRARY_PATH: ${DYLD_LIBRARY_PATH}" 1>> "${2}"
echo "${1} 1>> ${2} 2>&1 3>&1" 1>> "${2}"

# Perform the test
#
"${1}" 1>> "${2}" 2>&1 3>&1

RESULT=$?

# If test failed, then dump the log output. Otherwise create the `OK` file
#
if [[ ${RESULT} -ne 0 ]]; then
    cat "${2}"
else
    touch "${3}"
fi

exit ${RESULT}
