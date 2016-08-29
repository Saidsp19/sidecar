#!/bin/bash

# Run a unit test, recording any output to a temporary file. If the test fails, dump out the temporary file.
#

# Run test, redirecting stdout, stderr, and stdlog to a temporary file.
#
rm -f "${2}" "${3}"

# !!! Hack !!!
#
# For the MatlabBridge component, we need to have some MATLAB libraries in our path. However, we don't want to
# polute our LIBRARY_PATH env variable with it all the time because the MATLAB directory contains some stale
# versions of libraries we do use. The DYLD_... variable is the name used on Mac OS X.
#
# if [[ -n "${MATLAB_ROOT_LIB}" ]]; then
#     LD_LIBRARY_PATH="${MATLAB_ROOT_LIB}:${LD_LIBRARY_PATH}"
#     DYLD_LIBRARY_PATH="${MATLAB_ROOT_LIB}:${DYLD_LIBRARY_PATH}"
# fi

LD_LIBRARY_PATH="${PWD}:${LD_LIBRARY_PATH}"
DYLD_LIBRARY_PATH="${PWD}:${DYLD_LIBRARY_PATH}"

echo "LD_LIBRARY_PATH: ${LD_LIBRARY_PATH}" 1> "${2}"
echo "DYLD_LIBRARY_PATH: ${DYLD_LIBRARY_PATH}" 1> "${2}"
echo "${1} 1>> ${2} 2>&1 3>&1"

"${1}" 1>> "${2}" 2>&1 3>&1

RESULT=$?

if [[ ${RESULT} -ne 0 ]]; then
    cat "${2}"
else
    touch "${3}"
fi

exit ${RESULT}
