#!/usr/bin/env python

import cPickle, datetime, os
from struct import unpack
from array import array
import PRIChecker, Table

def align( fd, start, boundary ):
    offset = fd.tell()
    remaining = ( offset - start ) % boundary
    if remaining:
        fd.seek( remaining, 1 )

def getPickledPath( priPath ):
    return os.path.extsep.join( [ os.path.splitext( priPath )[ 0 ], 'pkl' ] )

class PRIRecord( list ):

    kDupeFlag = '+'
    kGapFlag = '-'

    def __init__( self, *values ):
        list.__init__( self, values )

    def getFlags( self ):
        if hasattr( self, 'flags' ):
            return getattr( self, 'flags' )
        return None

    def getFlag( self, flag ):
        if hasattr( self, 'flags' ):
            flags = getattr( self, 'flags' )
            if ( flag + self.kDupeFlag ) in flags:
                return flag + self.kDupeFlag
            if ( flag + self.kGapFlag ) in flags:
                return flag + self.kGapFlag
            if flag in flags:
                return flag
        return ' '

    def setFlag( self, flag, kind = '' ):
        if hasattr( self, 'flags' ):
            getattr( self, 'flags' ).add( flag + kind )
        else:
            setattr( self, 'flags', set( [ flag + kind ] ) )

    def setDupeFlag( self, flag ): self.setFlag( flag, self.kDupeFlag )
    def setGapFlag( self, flag ): self.setFlag( flag, self.kGapFlag )
    def getMessageSequence( self ): return self[ 0 ]
    def getCreateTimeStamp( self ): return self[ 1 ], self[ 2 ]
    def getEmittedTimeStamp( self ): return self[ 3 ], self[ 4 ]
    def getVMEMessageDescriptor( self ): return self[ 5 ]
    def getVMETimeStamp( self ): return self[ 6 ]
    def getVMESequenceCounter( self ): return self[ 7 ]
    def getVMEShaftEncoding( self ): return self[ 8 ]
    def getAzimuth( self ): return self.getVMEShaftEncoding() * 360.0 / 65536.0
    def getVMEIRIGTime( self ): return self[ 9 ]
    def getSampleCount( self ): return self[ 10 ]

    def __repr__( self ):
        return '<PRIRecord %d %d>' % ( self.getMessageSequence(),
                                       self.getVMESequenceCounter() )
    def __hash__( self ): return hash( self.getMessageSequence() )
    def __len__( self ): return self.getSampleCount()

class PRIRecords( object ):

    kAnomalyTable = Table.Table(
        0,
        ( '%6d', 'Index' ),
        ( '%7d', 'Seq #' ),
        ( '%5d', 'Shaft' ),
        ( '%6.2f', 'Az' ),
        ( '%17.6f', 'IRIG' ),
        ( '%12.6f', 'Offset' ),
        ( '%2s', PRIChecker.SequenceCounterChecker.kFlag ),
        ( '%2s', PRIChecker.ShaftEncodingChecker.kFlag ),
        ( '%2s', PRIChecker.IRIGTimeChecker.kFlag ),
        ( '%2s', PRIChecker.HighFreqencyCounterChecker.kFlag ),
        ( '%1s', PRIChecker.SampleCountChecker.kFlag ),
        )

    def __init__( self, priPath ):
        self.priPath = priPath
        self.payloadSize = None
        self.producerName = None
        self.messageType = None
        self.rangeMin = None
        self.rangeFactor = None
        self.records = None
        self.checkers = ( PRIChecker.SequenceCounterChecker(),
                          PRIChecker.ShaftEncodingChecker(),
                          PRIChecker.IRIGTimeChecker(),
                          PRIChecker.HighFreqencyCounterChecker(),
                          PRIChecker.SampleCountChecker() )
        self.anomalies = set()

    def getStartTime( self ): return self.records[ 0 ].getVMEIRIGTime()
    def getEndTime( self ): return self.records[ -1 ].getVMEIRIGTime()

    def check( self ):
        self.anomalies = set()
        for checker in self.checkers:
            checker.check( self )
        self.save()

    def getReportPath( self ):
        return os.path.extsep.join(
            ( os.path.splitext( self.priPath )[ 0 ], 'priinfo' ) )

    def hasReport( self ):
        return os.path.exists( self.getReportPath() )

    def generateReport( self ):
        reportPath = self.getReportPath()
        fd = open( reportPath, 'w' )
        write = fd.write
        write( ' Report Time: %s\n' % str( datetime.datetime.now() ) )
        write( 'Record Count: %d\n' % len( self.records ) )
        write( '   Anomalies: %d\n' % len( self.anomalies ) )
        if len( self.records ) == 0:
            return 0

        startTime = self.records[ 0 ].getVMEIRIGTime()
        endTime = self.records[ -1 ].getVMEIRIGTime()
        write( '\nStart Time: %17.6f\n' % startTime )
        write( '  End Time: %17.6f\n' % endTime )
        write( '  Duration: %14.3f\n' % ( endTime - startTime ) )
        if len( self.anomalies ) == 0:
            return 0

        write( '\nAnomaly Summary\n'
               '  #: sequence counter  I: IRIG time  E: shaft encoding'
               '  Z: sample size\n'
               '  -: gap/drop  +: duplicate value\n\n' )

        self.kAnomalyTable.printHeader( fd )
        for record in self.anomalies:
            flags = record.getFlags()
            self.kAnomalyTable.printValues(
                fd,
                record.getMessageSequence(),
                record.getVMESequenceCounter(),
                record.getVMEShaftEncoding(),
                record.getAzimuth(),
                record.getVMEIRIGTime(),
                record.getVMEIRIGTime() - startTime,
                record.getFlag( PRIChecker.SequenceCounterChecker.kFlag ),
                record.getFlag( PRIChecker.ShaftEncodingChecker.kFlag ),
                record.getFlag( PRIChecker.IRIGTimeChecker.kFlag ),
                record.getFlag( PRIChecker.HighFreqencyCounterChecker.kFlag ),
                record.getFlag( PRIChecker.SampleCountChecker.kFlag ),
                )
        write( '\n' )
        for checker in self.checkers:
            checker.printReport( fd )

        return len( self.anomalies )

    def save( self ):
        cPickle.dump( self, file( getPickledPath( self.priPath ), 'wb' ) )

    def foundAnomaly( self, record ):
        self.anomalies.add( record )

    def getAnomalies( self ): return sorted( self.anomalies )

    def load( self, fd, start = 0, count = -1 ):
        if count > 0:
            records = [ None ] * count
        else:
            records = []

        counter = 0
        past = None
        orderFlag = None
        while 1:
            pos = fd.tell()
            tmp = fd.read( 4 )
            if len( tmp ) != 4:
                break

            bits = unpack( 'HH', tmp )
            if bits[ 0 ] != 0xAAAA:
                raise RuntimeError, 'invalid magic value - ' + hex( bits[ 0 ] )

            if orderFlag is None:
                if bits[ 1 ] == 0:
                    orderFlag = '>'
                else:
                    orderFlag = '<'

            record = self.readOne( counter, fd, pos, orderFlag )
            if self.messageType != 2:
                self.records = []
                return

            if counter >= start:
                if len( records ) == 0:
                    pos = fd.tell()
                    fd.seek( 0, 2 )
                    fileSize = fd.tell()
                    fd.seek( pos, 0 )
                    records = [ None ] * ( fileSize / self.payloadSize )

                records[ counter - start ] = record

                if past is not None and len( self.checkers ) > 0:
                    for checker in self.checkers:
                        checker.checkPairs( self, past, record )
                past = record

            counter += 1
            if count != -1 and counter == count:
                break

        if len( records ) > counter:
            records = records[ : counter ]

        self.records = records

        for checker in self.checkers:
            checker.checkAll( self )

    def readOne( self, counter, fd, start, orderFlag ):
        first = self.payloadSize is None

        group1 = unpack( orderFlag + 'IHHI', fd.read( 12 ) )
        if first:
            self.payloadSize = group1[ 0 ]

        tmp = fd.read( group1[ 3 ] )
        if first:
            self.producerName = tmp

        align( fd, start, 2 )
        messageType = unpack( orderFlag + 'H', fd.read( 2 ) )[ 0 ]
        if first:
            self.messageType = messageType

        align( fd, start, 4 )
        group2 = unpack( orderFlag + '5IH2x5I',
                         fd.read( 5 * 4 + 2 + 2 + 5 * 4 ) )

        align( fd, start, 8 )
        if group2[ 5 ] < 4:
            group3 = ( unpack( orderFlag + '1d', fd.read( 8 ) )[ 0 ],
                       0.0, 0.125 )
        else:
            group3 = unpack( orderFlag + '3d', fd.read( 8 * 3 ) )

        if first:
            self.rangeMin = group3[ 1 ]
            self.rangeFactor = group3[ 2 ]

        sampleCount = unpack( orderFlag + 'I', fd.read( 4 ) )[ 0 ]

        if sampleCount > 6000:
            raise RuntimeError, 'suspect sampleCount value ' + \
                str( sampleCount )

        data = fd.read( 2 * sampleCount )
        sampleCount = len( data ) / 2

        return PRIRecord( counter, group2[ 1 ], group2[ 2 ], group2[ 3 ],
                          group2[ 4 ], group2[ 6 ], group2[ 7 ], group2[ 8 ],
                          group2[ 9 ], group3[ 0 ], sampleCount )

    def __len__( self ): return len( self.records )
    def __iter__( self ): return iter( self.records )
    def __getitem__( self, index ): return self.records[ index ]

def load( priPath, start = 0, count = -1, noCache = False ):
    pickledPath = getPickledPath( priPath )
    if os.path.exists( pickledPath ) and noCache is False:
        print '-- reading', priPath, 'from cache'
        obj = cPickle.load( file( pickledPath, 'rb' ) )
        obj.priPath = priPath
        return obj

    print '-- reading', priPath
    obj = PRIRecords( priPath )
    obj.load( open( priPath, 'rb' ), start, count )
    obj.save()

    return obj
