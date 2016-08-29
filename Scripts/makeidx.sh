#!/bin/bash

#
# Make index files for all PRI files found under a given directory (including
# subdirectories)
#
# usage: makeidx [-f] [-j#] DIR1 DIR2 ...
#
# where -f will force new indices, and -j sets the number of child processes to
# run to do the indexing (default is 6).
#
# Example: makeidx -j8 /space1/recordings/200910*
#

FORCE=""
let JOB_MAX=6
let JOB_COUNT=0
let JOB_INDEX=1

while getopts "fj:" OPT; do
    case "${OPT}" in
	f) FORCE="1" ;;
	j) let JOB_MAX=${OPTARG};;
    esac
done

. /opt/sidecar/sidecar.sh
POSER="$(type -P poser)"

function Abort # message
{
    echo "*** ${@}"
    exit 1
}

function ProcessFile # PATH
{
    local PATH="${1}"
    
    #
    # Try an strip off a 'pri' extension.
    #
    NAME="${PATH%.pri}"
    [[ "${NAME}" = "${PATH}" ]] && return

    #
    # It is a PRI file. See if there are already index files associated with
    # it.
    #
    [[ -f "${NAME}.timeIndex${FORCE}" &&
       -f "${NAME}.recordIndex${FORCE}" ]] && return
    

    #
    # Remove any old indices and process the file.
    #
    /bin/rm -f "${NAME}.timeIndex" "${NAME}.recordIndex"
    echo "...processing file ${1}"
    ${POSER} -i ${1} &

    #
    # Only allow a maximum of 8 jobs at a time. Wait for the oldest to finish
    # before continuing.
    #
    let JOB_COUNT+=1
    if (( JOB_COUNT - JOB_INDEX >= JOB_MAX - 1 )); then
	wait %${JOB_INDEX}
	let JOB_INDEX+=1
    fi
}

function ProcessDirectory # PATH
{
    echo "...processing directory ${1}"
    pushd "${1}" > /dev/null 2>&1 || Abort "failed to move into ${1}"
    ProcessItems *
    popd > /dev/null 2>&2
}

function ProcessItems # PATH [ PATH ... ]
{
    while [[ ${#} != 0 ]]; do
	if [[ -f "${1}" ]]; then
	    ProcessFile "${1}"
	elif [[ -d "${1}" ]]; then
	    ProcessDirectory "${1}"
	fi
	shift 1
    done
}

ProcessItems "${@}"

exit 0
