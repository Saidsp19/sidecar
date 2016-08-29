#!/bin/bash
#
# process [-d] [-f] [-t TEMPLATE_DIR] PATH [PATH...]
#
# Batch process PRI files in a given directory using a set of XML template
# files and a 'steps.sh' file that defines the processing steps to execute on
# the recorded data. Each step is described by a template XML file -- a valid
# XML file that ends with the '.template' suffix and contains a valid SideCar
# `runner' processing definition. Within these template files, all occurrences
# of the string %DIR% will change to the recording directory being processed.
# Additional substitutions may be accomplished depending on the definition of
# the template file(s) and the 'steps.sh' file.
#
# The 'steps.sh' file defines the sequence of processing steps to take on the
# recorded data. Each step should begin with the word 'Process' which is the
# name of a function defined below. The Process function accepts the following
# arguments:
#
#   NAME -- the name of the processing step and the base name of the template
#   XML file to use. For instance a NAME of 'AUX' will require the presence of
#   a template called AUX.xml.template in the template directory.
#
#   -o OUT -- an optional argument that specifies the output name to use for
#   the final XML file. If not present, the output name will be the same as the
#   input name (sans the '.template' suffix). This is useful when processing
#   multiple channels using the same XML template with unique output names.
#
#   NAME VALUE -- additional optional substitutions to apply to the template
#   file using the `sed' utility. Must occur as multiples of two, with the
#   first, NAME, defining the token in the template file to substitute
#   (surrounded by '%' characters by convention) and the second, VALUE,
#   defining the replacement value in the substitution. The substitution %DIR%
#   is automatically handled by the Process function; all occurrences of it in
#   the template XML file will changed to the path of the recording directory
#   begin processed.
#
# Here is a 'steps.sh' sequence that defines a simple block extraction process
# using negative video data:
#
#   Process NCIntegrate
#   Process Threshold
#   Process MofN
#   Process Extract
#
# The above assumes the presence of template files such as
# NCIntegrate.xml.template, Threshold.xml.template, etc.
#

[[ -z "${SIDECAR}" ]] && . /opt/sidecar/sidecar.sh

#
# Print out an error message and exit with a non-zero status
#
function Abort # message
{
    echo "*** $@"
    exit 1
}

DEBUG=""
TDIR="${PWD}"			# The directory containing the template files
RDIR=""				# The recording directory to process
FORCE=""			# Force reprocessing of all steps
STEPS=""

#
# Process the command-line options
#
while getopts 'dfs:t:' OPT; do
    case "${OPT}" in
	'd') DEBUG=1 ;;
	't') TDIR="${OPTARG}" ;;
	'f') FORCE="-f" ;;
	's') STEPS="${OPTARG}" ;;
	'?') exit 1;;
	'') break ;;
    esac
done

let OPTIND="((${OPTIND}-1))"
shift $OPTIND

#
# Validate option values
#
[[ -d "${TDIR}" ]] || Abort "invalid template directory - ${TDIR}"
[[ -z "${STEPS}" ]] && STEPS="${TDIR}/steps.sh"
[[ -f "${TDIR}/steps.sh" ]] || Abort "no 'steps.sh' in template directory"

[[ -z "${@}" ]] && set -- "${PWD}"

#
# Generate an XML file from a template and execute it using `runner'
#
function Process # [-f] NAME [-o OUT] [SUB...]
{
    local FORCE_FLAG="${FORCE}"
    if [[ "${1}" = "-f" ]]; then
	FORCE_FLAG="-f"
	shift 1
    fi

    local IN="${1}"
    shift
    [[ -z "${IN}" ]] && Abort "empty NAME value for template"

    #
    # Generate an output name or use one given to us
    #
    local OUT="${IN}"
    if [[ "${1}" = "-o" ]]; then
	OUT="${2}"
	shift 2
	[[ -z "${OUT}" ]] && Abort "empty OUT value"
    fi

    #
    # Generate list of substitutions for `sed' to apply to the template file.
    #
    local SUBS="-e s!%DIR%!${RDIR}!g"
    while [[ $# -gt 1 ]]; do
	SUBS="${SUBS} -e s!${1}!${2}!g"
	shift 2
    done

    #
    # Generate template and XML file names
    #
    IN="${TDIR}/${IN}.xml.template"
    [[ -f "${IN}" ]] || Abort "template '${IN}' does not exist"
    OUT="${RDIR}/${OUT}.xml"

    #
    # Skip the step if it has already been processed and we are not forced to
    # reprocess it.
    #
    if [[ -f "${OUT}.DONE${FORCE_FLAG}" ]]; then
	echo "...skipping '${OUT}' processing"
	return
    fi

    echo "...generating '${OUT}' from '${IN}'"
    [[ -n "${DEBUG}" ]] && echo "sed ${SUBS} ${IN} > ${OUT}"
    sed ${SUBS} ${IN} > ${OUT} || \
	Abort "failed to create XML file from template '${IN}'"

    echo "...executing '${OUT}'"
    [[ -n "${DEBUG}" ]] && echo "runner 1 "${OUT}" > ${OUT}.log 2>&1"
    let BEGIN="$(date '+%s')"
    runner 1 "${OUT}" > ${OUT}.log 2>&1 || \
	Abort "failed to process '${OUT}' - check '${OUT}.log'"
    let END="$(date '+%s')"
    let DELTA="((END-BEGIN))"
    let MINS="((DELTA/60))"
    let SECS="((DELTA-MIN*60))"
    echo "...took ${MINS} min ${SECS} secs"

    echo "touch ${OUT}.DONE"
    touch "${OUT}.DONE"
}

#
# Process one or more recording directories by executing the steps found in the
# 'steps.sh' file in the template directory. The value of RDIR will change to
# the current recording directory.
#
for RDIR in "${@}"; do

    [[ -d "${RDIR}" ]] || Abort "recording directory '${RDIR}' does not exist"

    #
    # Execute the processing steps spelled out in the 'steps.sh' file
    #
    . ${TDIR}/steps.sh || Abort "failed to process '${RDIR}'"

    echo "...generating/updating index files"
    [[ -n "${DEBUG}" ]] && echo "makeidx ${FORCE_FLAG} ${RDIR}"
    makeidx ${FORCE_FLAG} "${RDIR}" || \
	Abort "failed to generate index files"
done

exit 0
