#!/usr/bin/env python

import re

class ColumnInfo( object ):

    def __init__( self, width, format, heading ):
        self.format = format
        front = ( width - len( heading ) ) / 2
        self.heading = ' ' * front + heading + ' ' * ( width - front -
                                                       len( heading ) )
        self.dashes = '-' * width

    def printHeader( self, fd, line ):
        if line == 1:
            fd.write( self.heading )
        elif line == 0:
            fd.write( self.dashes )
        else:
            fd.write( ' ' * len( self.dashes ) )
        fd.write( ' ' )

    def printValue( self, fd, value ):
        if value is None:
            fd.write( self.dashes )
        else:
            fd.write( self.format % value )
        fd.write( ' ' )

class Table( object ):

    kWidthRE = re.compile( '%[-]?([1-9][0-9]*)' )

    def __init__( self, indent = 0, *defs ):
        if not isinstance( indent, int ):
            raise ValueError, 'invalid indent value: ' + `indent`
        
        self.indent = indent
        self.columns = []
        for each in defs:
            self.addColumn( each )

    def addColumn( self, each ):
        width = int( self.kWidthRE.search( each[ 0 ] ).group( 1 ) )
        self.columns.append( ColumnInfo( width, each[ 0 ], each[ 1 ] ) )

    def setIndent( self, indent ):
        self.indent = indent

    def printHeader( self, fd ):
        for line in range( 1, -1, -1 ):
            if self.indent:
                fd.write( ' ' * ( self.indent ) )
            for each in self.columns:
                each.printHeader( fd, line )
            fd.write( '\n' )

    def printValues( self, fd, *values ):
        if self.indent:
            fd.write( ' ' * self.indent )
        for index in range( len( self.columns ) ):
            self.columns[ index ].printValue( fd, values[ index ] ),
        fd.write( '\n' )
