# -*- Mode: Python -*-
#
# optargs -- general-purpose argument processing class
#

import getopt, os, string, sys

#
# Options Class -- returns optional values. If an option is not present,
# returns None.
#
class Options:

    #
    # __init__ -- initialize instance with list of options from
    # `getopt()'. Handles both long and short names.
    #
    def __init__( self, opts, optDefs ):
        dict = self.__dict__
        for each in opts:
            another = None

            #
            # Strip off leading '-' characters
            #
            name, value = each
            if name[ 1 ] == '-':
                name = name[ 2 : ]

                #
                # Locate equivalent short option name
                #
                for each in optDefs:
                    if each[ 1 ] == name:
                        another = each[ 0 ]
                        break

            else:
                name = name[ 1 ]

                #
                # Locate equivalent long option name
                #
                for each in optDefs:
                    if each[ 0 ] == name:
                        another = each[ 1 ]
                        break

            #
            # For options w/out values, provide a TRUE value
            #
            if value == '':
                value = 1

            dict[ name ] = value
            if another:
                dict[ another ] = value

    #
    # __getattr__ -- Python get attribute hook. Called when attribute not
    # found in instance. Return a FALSE value.
    #
    def __getattr__( self, key ):
        if key[0] == '_':             # Disregard Python hooks
            raise AttributeError(key)
        return None
    
    #
    # __getitem__ -- Python indexing hook. Returns the value for the
    # given key. Useful when there are non-alphanumeric characters in the
    # option name.
    #
    def __getitem__( self, key ):
        d = self.__dict__
        return ( key in d and d[key] ) or None

    def get( self, key, value = None ):
        return self.__dict__.get( key, value )

#
# Arguments Class -- simplest class definition there is. Will hold
# and return argument values as instance attributes.
#
class Arguments:
    pass

#
# Usage -- creates a usage printout based on the supplied option and
# argument definitions. Optionally displays an error message.
#
def Usage( progName, optDefs, argDefs, err, fd = sys.stderr ):
    rc = 0
    write = fd.write
    if err:
        write( '*** ERR: %s ***\n' % err )
        rc = 1

    #
    # Spit out a command line with all options
    #
    write( 'Usage: %s [-?|--help]' % progName )
    for each in optDefs:
        write( ' [' )
        if each[ 0 ]:
            write( "-%s" % each[ 0 ] )
            if each[ 1 ]:
                write( '|' )
        if each[ 1 ]:
            write( "--%s" % each[ 1 ] )
        if each[ 2 ]:
            write( " %s" % each[ 2 ] )
        write( ']' )

    #
    # Spit out all argument names
    #
    for each in argDefs:
        write( ' %s' % string.upper( each[ 0 ] ) )

    write( '\n-?|--help : print out this message\n' )

    #
    # Spit out descriptions of options
    #
    for each in optDefs:
        if each[ 0 ]:
            write( '-%s' % each[ 0 ] )
            if each[ 1 ]:
                write( '|' )
        if each[ 1 ]:
            write( '--%s ' % each[ 1 ] )
        write( ': %s\n' % each[ 3 ] )

    #
    # Do likewise for arguments
    #
    for each in argDefs:
        write( '%s : %s\n' % ( string.upper( each[ 0 ] ), each[ 1 ] ) )

    sys.exit( rc )

def Error( optDefs, argDefs, err, fd = sys.stderr ):
    Usage( os.path.basename( argv[ 0 ] ), optDefs, argDefs, err, fd )

#
# Process -- analyze the given list of arguments using the given option
# and argument definitions. Returns a 2-tuple containing instances of
# Options and Arguments that represent the found values. If an error
# occurs, prints out a usage statement using the option and argument
# definitions.
#
def Process( optDefs, argDefs, argv = sys.argv ):
    progName = os.path.basename( argv[ 0 ] )

    opts = '?'

    #
    # Build up short option string and list of long option names for the
    # `getopt' routine.
    #
    longOpts = [ 'help' ]
    for each in ( optDefs or () ):

        #
        # Any short option name?
        #
        if each[ 0 ]:
            opts = opts + each[ 0 ]
            if each[ 2 ]:
                opts = opts + ':'       # Requires an argument

        #
        # Any long option name?
        #
        if each[ 1 ]:
            if each[ 2 ]:
                longOpts.append( each[ 1 ] + '=' ) # Requires an argument
            else:
                longOpts.append( each[ 1 ] )

    #
    # Let getopt do the dirty work.
    #
    try:
        opts, argv = getopt.getopt( argv[ 1 : ], opts, longOpts )
    except getopt.error as err:
        Usage(progName, optDefs, argDefs, err)

    #
    # Options have been stripped off. Now assign mandatory arguments.
    #
    opts = Options( opts, optDefs )

    #
    # Show usage if asked for help.
    #
    if opts[ '?' ] or opts.help:
        Usage( progName, optDefs, argDefs, None )

    args = Arguments()
    for each in ( argDefs or () ):
        try:
            setattr( args, each[ 0 ], argv[ 0 ] )
            argv = argv[ 1 : ]
        except IndexError:
            Usage( progName, optDefs, argDefs,
                   "missing required argument " + string.upper( each[ 0 ] ) )

    #
    # Save any remaining arguments
    #
    args.argv = argv

    return opts, args
