#!/usr/bin/env python

from __future__ import print_function

import os, string, sys
import xml.dom.minidom

if 'SIDECAR_SRC' in os.environ:
    path = os.path.join( os.environ['SIDECAR_SRC'], 'lib')
else:
    path = os.path.join( os.environ['SIDECAR'], 'lib')

sys.path.insert(0, path)

import optargs, Logger

def Abort(msg):
    print(msg)
    sys.exit( 1 )

class InputOutput:

    kTag = None

    def __init__(self, node):
        self.name = node.getAttribute( 'name' )
        self.type = node.getAttribute( 'type' )
        self.channel = node.getAttribute( 'channel' )

    def generateC( self ):
        pass

    def generateXML( self ):
        if self.name == '' and self.channel == '':
            print('<%s type="%s"/>' % (self.kTag, self.type,))
        elif self.name == '':
            print('<input type="%s" channel="%s"/>' % (self.kTag, self.type, self.channel))
        elif self.channel == '':
            print('<input type="%s" name="%s"/>' % (self.kTag, self.type, self.name))
        else:
            print('<input type="%s" name="%s" channel="%s"/>' % (self.kTag, self.type, self.name, self.channel))

class Input( InputOutput ):
    kTag = 'input'
    
class Output( InputOutput ):
    kTag = 'output'

class Parameter:

    def __init__( self, node ):
        self.name = node.getAttribute( 'name' )
        self.type = node.getAttribute( 'type' )
        self.value = node.getAttribute( 'value' )

    def generateC( self ):
        name = 'kDefault' + self.name[ 0 ].upper() + self.name[ 1 : ]
        value = self.value
        type = self.type
        if type == 'string':
            type = 'char* const'
            value = '"' + value + '"'
        elif type == 'boolean':
            type = 'bool'
        print("static const", type, name, '=', value, ';')

    def generateXML(self):
        print('<param name="%s" type="%s" value="%s"/>' % (self.name, self.type, self.value))

class Algorithm:

    def __init__( self, node ):
        self.dll = node.getAttribute('dll')
        self.name = node.getAttribute('name')
        if self.name == '':
            self.name = self.dll

        self.inputs = []
        for input in node.getElementsByTagName( 'input' ):
            self.inputs.append(Input(input))

        self.outputs = []
        for output in node.getElementsByTagName('output'):
            self.outputs.append(Output(output))

        self.parameters = []
        for parameter in node.getElementsByTagName('param'):
            self.parameters.append(Parameter(parameter))

    def generateC( self ):
        for parameter in self.parameters:
            parameter.generateC()

    def generateXML( self ):
        print('<algorithm name="%s" dll="%s">' % (self.name, self.dll))
        for input in self.inputs:
            input.generateXML()
        for output in self.inputs:
            outputs.generateXML()
        for parameters in self.inputs:
            parameters.generateXML()
        print('</algorithm>')

    def dump( self ):
        print('    Algorithm:', self.name)

class Configuration:

    def __init__( self, node ):
        self.name = node.getAttribute( 'name' )
        self.algorithms = {}
        for spec in node.getElementsByTagName( 'algorithm' ):
            algorithm = Algorithm( spec )
            self.algorithms[ algorithm.name ] = algorithm

    def generateC( self, name ):
        self.algorithms[ name ].generateC()

    def generateXML( self, name ):
        self.algorithms[ name ].generateXML()

    def dump( self ):
        print('  Configuration:', self.name)
        for algorithm in self.algorithms.values():
            algorithm.dump()

class Document:

    def __init__( self, node ):
        self.configurations = {}
        for spec in node.getElementsByTagName( 'configuration' ):
            configuration = Configuration( spec )
            self.configurations[ configuration.name ] = configuration

    def generateC( self, configuration, algorithm ):
        self.configurations[ configuration ].generateC( algorithm )

    def generateXML( self, configuration, algorithm ):
        self.configurations[ configuration ].generateXML( algorithm )

    def dump( self ):
        print('Document:')
        for configuration in self.configurations.values():
            configuration.dump()

optDefs = (
    ('a', 'algorithm', 'NAME', "name of the algorithm to emit"),
    ('c', 'config', 'NAME', "name of the configuration to emit"),
    ('f', 'format', 'FORMAT', "output format to use ('xml' or 'c' )"),
    ('o', 'output', 'FILE', "write output to file FILE"),)
argDefs = (
    ('input', 'path to file to process' ),)

def main():
    opts, args = optargs.Process(optDefs, argDefs)
    format = opts.get( 'format', 'c' )
    path = args.input
    if not os.path.exists( path ):
        optargs.Error( 'file does not exist', opts, args )

    config = opts.get( 'config', '' )

    algorithm = opts.get( 'algorithm' )
    if algorithm is None:
        algorithm = os.path.basename( path )
        algorithm = os.path.splitext( algorithm )[ 0 ]

    if opts.output:
        sys.stdout = open( opts.output, 'w' )

    dom = xml.dom.minidom.parse( path )
    document = Document( dom.getElementsByTagName( 'configurations' )[ 0 ] )
    document.generateC( config, algorithm )

if __name__ == '__main__':
    main()
