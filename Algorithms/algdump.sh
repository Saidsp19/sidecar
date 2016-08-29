#!/bin/bash

# Dumps basic parameter info about the algorithms
# misses some parameters, but gets most of the work done

# List all the algorithms
echo "<algorithms>"
echo
for alg in `grep ^AddAlgorithm Jamfile | cut -d' ' -f 2`; do
    # Find the main source files
    src=`grep ^doLib $alg/Jamfile | cut -d' ' -f 4`

    # Identify the "official" (DLL) name
    dll=`grep Make ${alg}/${src} | grep Controller | grep Logger | cut -d'(' -f 1 | head -n 1`
    dll=(${dll/Make})
    last=${#dll[@]}
    dll=${dll[last-1]}

    # Some algorithms input multiple types...
    # Some algorithms output multiple types...
    # Need a better specification format.
    echo "<algorithm name=\"$dll\">"
    echo "  <struct><!-- describe the parameters in an XML-RPC-friendly format -->"

    # Find all the parameters
    IFSBAK=$IFS
    IFSLINE=$'\n\b'
    IFS=$IFSLINE
    for param in `grep 'Parameter::.*::Make' $alg/$src`; do
	# variable name
	name=${param/Parameter*/}
	name=${name//[( $'\t']}

	# parameter type
	tmp=${param/*Parameter::/}
	type=${tmp/::*/}
	type=${type//[\/\*]}

	# user string
	tmp=${param/*\"}
	desc=`echo $param | cut -d'"' -f2`

	# decipher the parameter type
	case $type in
	    BooleanValue)
		ctype="bool"
		default="false"
		;;
	    DoubleValue)
		ctype="double"
		default=0
		;;
	    IntValue)
		ctype="int"
		default=0
		;;
	    PositiveIntValue)
		ctype="uint"
		default=0
		;;
	    InterpolationValue)
		ctype="double" # not a very good match
		default=0
		;;
	    ScanT)
		ctype="uint"
		default=0
		;;
	    ShortValue)
		ctype="int"
		default=0
		;;
	    *)
		ctype=$type
		default=""
		;;
	esac

	echo "    <member><name>$desc</name><value><$ctype>$default</$ctype></value></member>"

    done
    echo "  </struct>"

    # Find all the inputs
    echo "  <inputs>"
    for input in `grep 'messageStream\..*true' $alg/$src`; do
    type=${input/*messageStream.}
    type=${type/=*}
    type=${type//[ ]}

	echo "    <type>$type</type>"
    done
    echo "  </inputs>"

    # Find all the outputs
    echo "  <outputs>"
    for output in `grep 'Messages::.*::Make' $alg/$src`; do
	type=${output//*Messages::}
	type=${type/::*}

	echo "    <type>$type</type>"
    done
    segout=`grep SegmentMessage $alg/$src`
    if [[ ${#segout} -ge 2 ]]; then
	echo "    <type>SegmentMessage</type>"
    fi
    echo "  </outputs>"
    echo "</algorithm>"
    IFS=$IFSBAK
done
echo
echo "</algorithms>"
