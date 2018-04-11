#!/bin/bash
#
# Startup stub for SideCar GUI applications. Makes sure that certain environment variables are set before
# starting an application.
#
# Usage: startup [-r][-L LOGFILE] program [ARGS...]
#

LOG=""
RESTART_ON_FAILURE=""

function MacOSAlert # APP TEXT 
{
    cat << +EOF+ | /Applications/Pashua.app/Contents/MacOS/Pashua -
*.title = ${1}
tx.type = text
tx.text = ${2}
+EOF+
}

# Process any options
#
while [[ "${1#-}" != "${1}" ]]; do
    case "${1}" in
	"-L")
	    LOG="${2}"
	    shift 1
	    ;;

	"-r")
	    RESTART_ON_FAILURE="Y"
	    ;;

	"*")
	    echo "*** invalid option - ${1} ***"
	    exit 1
	    ;;

    esac
    shift 1
done

[[ -z "${LOG}" ]] && LOG="/tmp/${1}_${USER}.log"

# If there is a user's ${HOME}/.profile, bring it in first so we can get SIDECAR set
#
PROFILE="${HOME}/.profile"
[[ -f ${PROFILE} ]] && . ${PROFILE}

# Attempt to remove any existing file there. However, if the file remains, assume we cannot write to it and use
# /dev/null instead.
#
if [[ "${LOG}" != "-" ]]; then
    rm -rf "${LOG}"
    if [[ -f "${LOG}" ]]; then
	echo "WARNING: unable to write to ${LOG} -- using /dev/null"
	LOG="/dev/null"
    else
	touch "${LOG}"
	chmod a+rw "${LOG}"
    fi
fi

# Now bring in the SideCar environment.
#
[[ -f "${SIDECAR}/bin/env-init" ]] && . "${SIDECAR}/bin/env-init" ""

# Start the requested program. If the program quits with a non-zero exit, restart it if asked to.
#
STATUS=1
let MAX_MEM_G=1			# 1 GIG
let MAX_MEM_M=${MAX_MEM_G}*1024
let MAX_MEM_K=${MAX_MEM_M}*1024

ulimit -d ${MAX_MEM_K}

# For developers, allow core files
#
[[ -n "${SIDECAR_SRC}" ]] && ulimit -c unlimited

APP="${1}"

while [[ "${STATUS}" != 0 ]]; do

    #
    # Close stdin, stdout, stderr
    #
    exec 1>&-
    exec 2>&-
    exec 3>&-

    if [[ "${LOG}" != "-" ]]; then
	OLD_UMASK="$(umask)"
	umask -S u=rw,g=rw,o=r
	touch "${LOG}"
	umask ${OLD_UMASK}
	"${@}" > "${LOG}" 2>&1 3>&1
    else
	"${@}"
    fi	
    STATUS="${?}"

    echo "Exit status: ${STATUS}" >> ${LOG}

    # set -x
    ALERT="${SIDECAR}/bin/alert"
    [[ ! -x "${ALERT}" ]] && ALERT=""
    [[ -z "${DISPLAY}" ]] && ALERT=""
    [[ "$(uname -s)" = "Darwin" ]] && ALERT=MacOSAlert

    if [[ -n "${ALERT}" ]]; then
	case "${STATUS}" in
	    0)
		;;
	    139)
		${ALERT} "${1}" "The application '${*}' was forced to exit because it used more than ${MAX_MEM_M} megabytes of memory."
		;;
	    *)
		${ALERT} "${1}" "The application '${*}' unexpectedly quit with an system exit code of ${STATUS}."
		;;
	esac
    fi
    [[ -z "${RESTART_ON_FAILURE}" ]] && break;
done

exit ${STATUS}
