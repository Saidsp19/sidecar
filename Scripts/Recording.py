#!/usr/bin/env python

import cPickle, glob, os, sys, time
import PRIChecker, PRIRecord, Table

def getPickledPath( directory ):
    return os.path.join( directory, 'recording.pkl' )

class BadTime:

    def __init__( self, file, anomaly ):
        self.file = file
        self.anomaly = anomaly
        self.sequenceCounter = anomaly.getVMESequenceCounter()
        self.irigTime = anomaly.getVMEIRIGTime()

    def getTime( self ): return self.irigTime

    def __hash__( self ):
        return hash( self.sequenceCounter )
    def __cmp__( self, other ):
        return cmp( self.sequenceCounter, other.sequenceCounter )
    def __repr__( self ):
        return '<BadTime %d %17.6f>' % ( self.sequenceCounter, self.irigTime )

class Recording( object ):

    kSpanTable = Table.Table(
        0,
        ( '%17.6f', 'Start Time' ),
        ( '%17.6f', 'End Time' ),
        ( '%14.3f', 'Duration' ),
        )

    def __init__( self, directory ):
        self.directory = directory
        self.files = []
        self.anomalousCount = 0
        self.startTime = None
        self.endTime = None
        self.badTimes = []

    def save( self ):
        cPickle.dump( self, file( getPickledPath( self.directory ), 'wb' ) )

    def addFile( self, priPath, recheck = False, noCache = False ):

        file = PRIRecord.load( priPath, noCache = noCache )

        if file.messageType != 2:
            print '-- invalid message type -', file.messageType
            return

        if recheck is True or not file.hasReport():
            print '-- generating anomaly report'
            found = file.generateReport()
            if found != 0:
                print '** found', found, 'anomalies'

        self.files.append( file )
        if len( file.anomalies ) > 0:
            self.anomalousCount += 1

        when = file.getStartTime()
        if self.startTime is None or self.startTime > when:
            self.startTime = when

        when = file.getEndTime()
        if self.endTime is None or self.endTime < when:
            self.endTime = when

        for anomaly in file.getAnomalies():
            self.badTimes.append( BadTime( file, anomaly ) )

    def printReport( self, fd, minAcceptableSpan = 5.0 ):
        fd.write( '   Recording: %s\n' % self.directory )
        fd.write( 'Number Files: %d\n' % len( self.files ) )
        fd.write( ' w/Anomalies: %d\n\n' % self.anomalousCount )
        fd.write( '  Start Time: %17.6f\n' % self.startTime )
        fd.write( '    End Time: %17.6f\n' % self.endTime )
        fd.write( '    Duration: %14.3f\n\n' % ( self.endTime -
                                               self.startTime ) )

        self.kSpanTable.printHeader( fd )
        startTime = self.startTime
        for badTime in self.badTimes:
            if badTime.getTime() - startTime >= minAcceptableSpan:
                self.kSpanTable.printValues( startTime, badTime.getTime(),
                                             badTime.getTime() - startTime )
            startTime = badTime.getTime()
        if self.endTime - startTime >= minAcceptableSpan:
            self.kSpanTable.printValues( startTime, self.endTime,
                                         self.endTime - startTime )

def load( directory, noCache = False ):
    recording = Recording( directory )
    for priPath in sorted( glob.glob( os.path.join( directory, '*.pri' ) ) ):
        recording.addFile( priPath, noCache = noCache )
    return recording

if __name__ == '__main__':
    for directory in sys.argv[ 1 : ]:
        print '-- loading', directory
        recording = load( directory )
        reportPath = os.path.join( directory, 'summary.txt' )
        recording.printReport( open( reportPath, 'w' ) )
        recording.printReport( sys.stdout )
