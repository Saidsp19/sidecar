#!/usr/bin/env python
#
# Script that will create a new algorithm from template files in the Algorithms
# directory.
#
# Usage: newalg NAME [PARAM1,PARAM2...] [IN1,IN2...] [OUT1,OUT2...]
#
# IN1 - TYPE[:NAME[:CHANNEL]]
# OUT1 - TYPE[:NAME[:CHANNEL]]
# PARAM - NAME:TYPE:INIT
#

import datetime, os, string, sys

kUsage = '''usage: newalg NAME [PARAM[,PARAM...]] [IN[,IN...]] [OUT[,OUT...]]
NAME - name of the new algorithm to create
PARAM - comma-separated list of parameter specifications with the format
        NAME:TYPE:INIT, where NAME is the parameter's short name, TYPE is the
        parameter's native data type (eg. int, double, or bool), and INIT is
        the parameter's initial value.
IN - comma-separated list of input channel specifications with the format
     TYPE[:NAME[:CHANNEL]], where TYPE is the message type of the channel,
     NAME is the optional name of the channel known by the algorithm,
     and CHANNEL is the optional name of the channel known to the rest of the
     system.
OUT - comma-separated list of output channel specifications with the format
     TYPE[:NAME[:CHANNEL]], where TYPE is the message type of the channel,
     NAME is the optional name of the channel known by the algorithm,
     and CHANNEL is the optional name of the channel known to the rest of the
     system.
'''

def Abort( msg ):
    print msg
    if __name__ == '__main__':
        sys.exit( 1 )
    raise RuntimeError

class NamedSet( list ):

    def __init__( self, typeName, contents ):
        names = set()
        for each in contents:
            if ( each.getName() in names ):
                Abort( '*** duplicate name in collection of ' + self.typeName )
            names.add( each.getName() )
        self.extend( contents ) 
        self.typeName = typeName
        self.contents = contents

class InputSet( NamedSet ):

    def __init__( self, contents ):
        NamedSet.__init__( self, 'inputs', contents )

class OutputSet( NamedSet ):

    def __init__( self, contents ):
        NamedSet.__init__( self, 'outputs', contents )


class ParamSet( NamedSet ):

    def __init__( self, contents ):
        NamedSet.__init__( self, 'parameters', contents )

def capitalizeFirst( value ):
    if len( value ) == 0: return value
    return value[ 0 ].upper() + value[ 1 : ]

class Connection( object ):

    kTypeIndex = 0
    kNameIndex = 1
    kChannelIndex = 2

    usedTypes = set()

    def __init__( self, spec ):
        self.bits = spec.split( ':' )
        if len( self.bits ) not in ( 1, 2, 3 ):
            Abort( "*** invalid Connection specification - " + spec )
        self.bits.extend( [ '' ] * ( 3 - len( self.bits ) ) )
        self.bits[ self.kTypeIndex ] = \
            capitalizeFirst( self.bits[ self.kTypeIndex ] )
        self.usedTypes.add( self.getType() )
        capName = capitalizeFirst( self.getName() )
        if len( capName ) == 0:
            capName = 'Input'
        self.subs = { 'name': self.getName(),
                      'capName': capName,
                      'typeName': self.getType() }

    def getType( self ): return self.bits[ self.kTypeIndex ]

    def getName( self ): return self.bits[ self.kNameIndex ]

    def getChannel( self ): return self.bits[ self.kChannelIndex ]

    def getXML( self, tag ):
        output = '   <' + tag + ' type="' + self.getType() + '"'
        if self.getName():
            output += ' name="' + self.getName() + '"'
        if self.getChannel():
            output += ' channel="' + self.getChannel() + '"'
        return output + '/>\n'

class Input( Connection ):

    def getXML( self ):
        return Connection.getXML( self, 'input' )

    def getProcessorDecl( self ):
        body = string.Template( '''
    /** Process messages from channel ${name}
        \\param msg the input message to process
        \\returns true if no error; false otherwise
    */
    bool process${capName}( const Messages::${typeName}::Ref& msg );\n''' )
        return body.substitute( self.subs )

    def getProcessorReg( self, classNameValue ):
        if len( self.getName() ) == 0:
            body = string.Template( '    registerProcessor<${className},'
                                    'Messages::${typeName}>( \n'
                                    '        &${className}::process${capName} '
                                    ');\n' )
        else:
            body = string.Template( '    registerProcessor<${className},'
                                    'Messages::${typeName}>( \n'
                                    '"${name}", '
                                    '&${className}::process${capName} );\n' )
        return body.substitute( self.subs, className = classNameValue );

    def getProcessorBody( self, classNameValue ):
        body = string.Template( '''bool
${className}::process${capName}( const Messages::${typeName}::Ref& msg )
{
    static Logger::ProcLog log( "process${capName}", getLog() );

    //
    // Create a new message to hold the output of what we do. Note that
    // although we pass in the input message, the new message does not contain
    // any data.
    //
    Messages::${typeName}::Ref out(
        Messages::${typeName}::Make( "${className}::process${capName}", msg ) );

    //
    // By default the new message has no elements. Either append them or
    // resize.
    //
    out->resize( msg->size() );

    //
    // Send out on the default output device, and return the result to our
    // Controller. NOTE: for multichannel output, one must give a channel index
    // to the send() method. Use getOutputChannelIndex() to obtain the index
    // for an output channel with a given name.
    //
    bool rc = send( out );
    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}
''' )
        return body.substitute( self.subs, className = classNameValue )

class Output( Connection ):

    def getXML( self ):
        return Connection.getXML( self, 'output' )

class Parameter( object ):

    kNameIndex = 0
    kTypeIndex = 1
    kValueIndex = 2

    def __init__( self, spec ):
        self.bits = spec.split( ':' )
        if len( self.bits ) != 3:
            Abort( '*** invalid Parameter specification - ' + spec )
        self.bits[ self.kNameIndex ] = self.bits[ self.kNameIndex ]
        self.bits[ self.kTypeIndex ] = \
            capitalizeFirst( self.bits[ self.kTypeIndex ] )

    def getName( self ): return self.bits[ self.kNameIndex ]

    def getType( self ): return self.bits[ self.kTypeIndex ]

    def getValue( self ): return self.bits[ self.kValueIndex ]

    def getXMLType( self ): 
        value = self.getType().lower()
        if value == 'bool':
            value = 'boolean'
        return value

    def getXMLValue( self ): 
        value = self.getValue()
        if self.getXMLType() == 'boolean':
            if value == "true":
                value = '1'
            else:
                value = '0'
        return value

    def getXML( self ):
        return '   <param name="%s" type="%s" value="%s"/>\n' % \
            ( self.getName(), self.getXMLType(), self.getXMLValue() )

    def getDecl( self ):
        return '    Parameter::%sValue::Ref %s_;\n' % \
            ( self.getType(), self.getName() )

    def getInit( self ):
        template = string.Template( ',\n      ${name}_( '
                                    'Parameter::${typeName}Value::Make(\n'
                                    '                    '
                                    '"${name}", "${capName}", '
                                    'kDefault${capName} ) )' )
        return template.substitute(
            name = self.getName(),
            typeName = self.getType(),
            capName = capitalizeFirst( self.getName() ) )

    def getReg( self ):
        return '    ok = ok && registerParameter( %s_ );\n' % \
            ( self.getName(), )

class Algorithm( object ):

    def __init__( self, name, params, inputs, outputs ):
        self.name = name[ 0 ].upper() + name[ 1 : ]
        self.ucName = name.upper()
        self.params = ParamSet( params )
        self.inputs = InputSet( inputs )
        self.outputs = OutputSet( outputs )
        self.subs = { 'name': self.name,
                      'ucName': self.ucName,
                      'year': datetime.date.today().year }

    def getName( self ): return self.name

    def generate( self ):
        try:
            algDir = os.path.join( os.environ[ 'SIDECAR_SRC' ], 'Algorithms' )
        except:
            Abort( '*** the environment variable SIDECAR_SRC is not set' )

        newDir = os.path.join( algDir, self.name );
        try:
            os.mkdir( newDir )
        except:
            Abort( '*** failed mkdir ' + newDir )

        try:
            oldDir = os.getcwd()
            try:
                os.chdir( newDir )
            except:
                Abort( '*** failed cd ' + newDir )

            if not self.generateHeader():
                Abort( '*** failed to generate include file' )

            if not self.generateBody():
                Abort( '*** failed to generate C++ file' )
            
            if not self.generateUnitTest():
                Abort( '*** failed to unit test skeleton' )

            if not self.generateJamfile():
                Abort( '*** failed to generate Jamfile file' )

            if not self.generateXML():
                Abort( '*** failed to generate AXML file' )

            self.updateAlgorithmsJamfile( algDir )
        finally:
            os.chdir( oldDir )

    def generateHeader( self ):
        body = string.Template(
            '''#ifndef SIDECAR_ALGORITHMS_${ucName}_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_${ucName}_H

//
// (C) Copyright ${year} Massachusetts Institute of Technology. All rights
// reserved.
//

#include "Algorithms/Algorithm.h"
${includes}#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm ${name}.
    Please describe what the algorithm does, in layman's terms and, if
    possible, mathematical terms.
*/
class ${name} : public Algorithm
{
    typedef Algorithm Super;
public:

    enum InfoSlots {
        kEnabled = ControllerStatus::kNumSlots,
        kNumSlots
    };

    /** Constructor.
        \param controller object that controls us
        \param log device used for log messages
    */
    ${name}( Controller& controller, Logger::Log& log );

    /** Implementation of the Algorithm::startup interface. Register runtime
        parameters and data processors.
        \\return true if successful, false otherwise
    */
    bool startup();

    /** Implementation of the Algorithm::shutdown interface. Dispose of any
        resources (such as memory) allocated from within the startup() method.
        \\return true if successful, false otherwise
    */
    bool shutdown();

private:

    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots( IO::StatusBase& status );
${processorDecls}
    //
    // Add attributes here
    //
${paramDecls}};

} // end namespace Algorithms
} // end namespace SideCar

/** \\file
 */

#endif
''' )

        includes = ''
        for each in Connection.usedTypes:
            includes += '#include "Messages/%s.h"\n' % each

        processorDecls = ''
        for each in self.inputs:
            processorDecls += each.getProcessorDecl()

        paramDecls = ''
        for each in self.params:
            paramDecls += each.getDecl()

        fd = file( self.name + '.h', 'w' )
        fd.write( body.substitute( self.subs,
                                   includes = includes,
                                   processorDecls = processorDecls,
                                   paramDecls = paramDecls ) )
        fd.close()
        return True

    def generateBody( self ):
        body = string.Template( '''//
// (C) Copyright ${year} Massachusetts Institute of Technology. All rights
// reserved.
//

#include <algorithm>		// for std::transform
#include <functional>		// for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "${name}.h"
#include "${name}_defaults.h"

#include <QtCore/QString>
#include <QtCore/QVariant>

using namespace SideCar;
using namespace SideCar::Algorithms;

//
// Constructor. Do minimal initialization here. Registration of processors and
// runtime parameters should occur in the startup() method. NOTE: it is WRONG
// to call any virtual functions here...
//
${name}::${name}( Controller& controller, Logger::Log& log )
    : Super( controller, log )${paramInits}
{
    ;
}

//
// Startup routine. This is called right after the Controller loads our DLL and
// creates an instance of the ${name} class. Place registerProcessor and
// registerParameter calls here. Also, be sure to invoke Algorithm::startup()
// as shown below.
//
bool
${name}::startup()
{
${processorRegs}    bool ok = true;
${paramRegs}    return ok && Super::startup();
}

bool
${name}::shutdown()
{
    //
    // Release memory and other resources here.
    //
    return Super::shutdown();
}

${processorBodys}
void
${name}::setInfoSlots( IO::StatusBase& status )
{
    status.setSlot( kEnabled, enabled_->getValue() );
}

extern "C" ACE_Svc_Export QVariant
FormatInfo( const IO::StatusBase& status, int role )
{
    if ( role != Qt::DisplayRole )
        return QVariant();

    if ( ! status[ ${name}::kEnabled ] ) return "Disabled";

    //
    // Format status information here.
    //
    return QString( "" );
}

//
// Factory function for the DLL that will create a new instance of the
// ${name} class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
${name}Make( Controller& controller, Logger::Log& log )
{
    return new ${name}( controller, log );
}
''' )

        paramInits = ''
        paramRegs = ''
        for each in self.params:
            paramInits += each.getInit()
            paramRegs += each.getReg()

        processorRegs = ''
        processorBodys = ''
        for each in self.inputs:
            processorRegs += each.getProcessorReg( self.getName() );
            processorBodys += each.getProcessorBody( self.getName() );

        fd = file( self.name + '.cc', 'w' )
        fd.write( body.substitute( self.subs,
                                   paramInits = paramInits,
                                   processorRegs = processorRegs,
                                   paramRegs = paramRegs,
                                   processorBodys = processorBodys ) )
        fd.close()
        return True

    def generateUnitTest( self ):
        body = string.Template( '''//
// (C) Copyright ${year} Massachusetts Institute of Technology. All rights
// reserved.
//

#include <ace/FILE_Connector.h>
#include <ace/Reactor.h>
#include <ace/Stream.h>

#include "Algorithms/Controller.h"
#include "Algorithms/ShutdownMonitor.h"
#include "IO/Readers.h"
#include "IO/FileWriterTask.h"
#include "IO/MessageManager.h"
#include "IO/Module.h"
#include "IO/ProcessingStateChangeRequest.h"
#include "IO/ShutdownRequest.h"

#include "Logger/Log.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "${name}.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj
{
    Test() : UnitTest::TestObj( "${name}" ) {}
    void test();
};

void
Test::test()
{
    Logger::Log::Root().setPriorityLimit( Logger::Priority::kDebug );

    //
    // Replace the following with a real unit test.
    //
    assertTrue( false );
}

int
main( int argc, const char* argv[] )
{
    return Test().mainRun();
}
''' )
        fd = file( self.name + 'Test.cc', 'w' )
        fd.write( body.substitute( self.subs ) )
        fd.close()
        return True

    def generateJamfile( self ):
        body = string.Template( '''# -*- Jam -*-
#
# Boost Build config file for the '${name}' algorithm
#
# (C) Copyright ${year} Massachusetts Institute of Technology. All rights
# reserved.
#

#
# Define algorithm name for building. Others could link to this by using the
# BoostBuild identifier /Algorithms//${name}
#
project ${name} ;

#
# Build specification for algorithm. Creates dependencies between the resulting
# DLL and source/object files, as well as any optional library dependencies.
#
doAlg ${name} : #
                # Add source files below.
                #
                ${name}.cc

              : #
                # Add any special build requirements below, such as special
                # <library>, <include>, or <define> specifications. Not usual.
                #

              : #
                # Default build type: debug profile release
                # This overrides the setting in Jamroot, which is debug
                #
              ;

#
# Unit test for the algorithm. Runs whenever the algorithm DLL builds.
#
doTest ${name}Test : ${name}Test.cc ${name} ;

''' )
        fd = file( 'Jamfile', 'w' )
        fd.write( body.substitute( self.subs ) )
        fd.close()
        return True

    def generateXML( self ):
        fd = file( self.name + '.axml', 'w' )
        fd.write( '''<?xml version="1.0"?>
<configurations>
 <configuration name="">
  <algorithm dll="%s">
''' % self.name )
        for each in self.inputs:
            fd.write( each.getXML() )

        for each in self.params:
            fd.write( each.getXML() )

        for each in self.outputs:
            fd.write( each.getXML() )

        fd.write( '''  </algorithm>
 </configuration>
</configurations>
''' )

        fd.close()
        return True

    def updateAlgorithmsJamfile( self, algDir ):

        #
        # Read in the current Jamfile file, remember existing algorithm names.
        #
        algs = {}
        algs[ self.name ] = 0

        before = ''
        fd = file( os.path.join( algDir, 'Jamfile' ), 'r' )
        for line in fd:
            trimmed = line.strip()
            if len( trimmed ) < 1:
                before += line
                continue

            isComment = 0
            if trimmed[ 0 ] == '#':
                trimmed = trimmed[ 1 : ].strip()
                isComment = 1

            bits = trimmed.split()
            if len( bits ) > 0 and bits[ 0 ] == 'AddAlgorithm':
                algs[ bits[ 1 ] ] = isComment
            else:
                before += line

        fd.close()

        #
        # Create a new Jamfile with the updated AddAlgorithm statements
        #
        fd = file( os.path.join( algDir, 'Jamfile' ), 'w+' )
        fd.write( before )

        names = algs.keys()
        names.sort( key = str.lower )
        for name in names:
            value = algs[ name ]
            line = 'AddAlgorithm %s ;\n' % name
            if value == 1:
                line = '# ' + line
            fd.write( line )

        fd.close()

def run( args ):

    args.extend( [ '' ] * 3 )
    name = args[ 0 ]
    if name == '':
        Abort( kUsage )

    if len( name.split() ) != 1:
        Abort( '*** name cannot contain spaces' )

    if len( args[ 1 ] ):
        params = map( Parameter, args[ 1 ].split(',') )
    else:
        params = []
    params.append( Parameter( 'enabled:bool:1' ) )
        
    if len( args[ 2 ] ):
        inputs = map( Input, args[ 2 ].split(',') )
    else:
        inputs = [ Input( 'Video' ) ]

    if len( args[ 3 ] ):
        outputs = map( Output, args[ 3 ].split(',') )
    else:
        outputs = []

    alg = Algorithm( name, params, inputs, outputs )
    alg.generate()

if __name__ == '__main__':
    run( sys.argv[1:] )
