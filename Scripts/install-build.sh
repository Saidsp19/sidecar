#!/bin/bash
#
# Install the SideCar build objects and configuration files into the ${SIDECAR}/builds directory using today's
# date as the name of the directory. Adds a suffix in case of multiple builds on the same day. Creates a
# symbolic called ${SIDECAR}/builds/latest that points to the newly created build directory. Also creates if not
# present a symbolic link from ${SIDECAR}/builds/latest/data to ${SIDECAR}/data.
# 

# Uncomment to show trace messages as Bash statements execute
#
# set -x

function Abort # MESSAGE
{
    echo "*** ${@} ***"
    exit 1
}

# Process optional arguments
#
SUFFIX=""
while [[ "${1#-}" != "${1}" ]]; do
    case "${1##-}" in
	d) set -x ;;
	r) REPLACE="1" ;;
	s) SUFFIX="${2}"; shift 1 ;;
	*) Abort "invalid option ${1}" ;;
    esac
    shift 1
done

# Verify a usable SIDECAR setting
#
[[ -n "${SIDECAR}" && -d "${SIDECAR}" ]] || Abort "invalid SIDECAR setting"

TOP="${PWD}"
BASE="${SIDECAR}/builds/$(date +%Y%m%d)${SUFFIX}"
DEST="${BASE}"

if [[ -n "${REPLACE}" ]]; then
    [[ -d "${DEST}" ]] && rm -rf "${DEST}"
else

    #
    # Obtain a unique directory name
    #
    let COUNT=1
    while [[ -d "${DEST}" ]]; do
	DEST="${BASE}-${COUNT}"
	let COUNT+=1
    done
fi

mkdir -p "${DEST}" || Abort "failed to create ${DEST}"

# Copy over the bin and lib files.
#
cp -r "${TOP}/bin" "${TOP}/lib" "${TOP}/data" "${DEST}" \
    || Abort "failed to install into ${DEST}"

# Install/update the env-init.sh and sidecar.sh files. Note that the latter is placed inside ${SIDECAR}.
#
if [[ -f "${TOP}/VERSION" ]]; then
    cp "${TOP}/VERSION" "${DEST}/" || Abort "failed to copy VERSION"
fi

cp "${TOP}/env-init.sh" "${DEST}/" || Abort "failed to install env-init.sh"
cp "${TOP}/sidecar.sh" "${SIDECAR}/" || Abort "failed to install sidecar.sh"

# Update the 'latest' symbolic link to point to the new build directory.
#
rm -f "${SIDECAR}/builds/latest"
ln -s "${DEST}" "${SIDECAR}/builds/latest"

# If missing, create a top-level symbolic link to the latest data directory.
#
[[ -d "${SIDECAR}/data" ]] || \
    ln -s "${SIDECAR}/builds/latest/data" "${SIDECAR}/data"

echo "installed into ${DEST}"

exit 0
