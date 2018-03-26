#
# --- Setup SideCar environment variables.
#

# Add a value to a ':' separated variable value (eg PATH) as long as it does not
# already exist in the given variable's value.
#
# \param VAR the name of the variable to change, such as PATH or LD_LIBRARY_PATH
# \param VALUE value to add
#
function PathAddOne # [-a] VAR VALUE
{
    local APPEND=""
    while [[ "${1#-}" != "${1}" ]]; do
	case "${1}" in
	    -a)
		APPEND="1"
		;;
	    *)
		echo "*** invalid option - '${1}' ***"
		;;
	esac
	shift 1
    done

    local OFS="${IFS}" VAR="${1}" VALUE="${2}"
    [[ "${ARCH}" = "Darwin" ]] && [[ "${VAR}" = "LD_LIBRARY_PATH" ]] && VAR="DY${VAR}"

    IFS=":"
    eval set -- \$${VAR}
    IFS="${OFS}"
    for EACH in "${@}"; do
	[[ "${EACH}" = "${VALUE}" ]] && return
    done

    # Add the value to the beginning of the variable.
    #
    if [[ -n "${APPEND}" ]]; then
	eval "${VAR}=\"\$${VAR}:${VALUE}\""
    else
	eval "${VAR}=\"${VALUE}:\$${VAR}\""
    fi
}

function PathAdd # VAR VALUE VALUE VALUE
{
    local APPEND=""
    while [[ "${1#-}" != "${1}" ]]; do
	case "${1}" in
	    -a)
		APPEND="-a"
		;;
	    *)
		echo "*** invalid option - '${1}' ***"
		;;
	esac
	shift 1
    done

    local VAR="${1}"
    shift 1
    for EACH in "${@}"; do
        PathAddOne ${APPEND} "${VAR}" "${EACH}"
    done
}

export SIDECAR
export SIDECAR_SRC
export SIDECAR_CONFIG

if [[ -z "${SIDECAR_SRC}" && -d "${HOME}/sidecar" ]]; then
    SIDECAR_SRC="${HOME}/sidecar"
    SIDECAR="${SIDECAR_SRC}/builds/latest"
fi

[[ -z "${SIDECAR}" && -d "/opt/sidecar/bin" ]] && SIDECAR="/opt/sidecar"
if [[ -d "${SIDECAR}" ]]; then
    PathAdd PATH "${SIDECAR}/bin"
    [[ -z "${SIDECAR_CONFIG}" ]] && SIDECAR_CONFIG="${SIDECAR}/data/radar_files/20t10scans.xml"
else
    echo "*** SIDECAR not defined or not found"
fi
