#!/usr/bin/env python

import glob, os, sys
import PRIRecord
import Table

class ShaftEncodingChecker:

    kDupeTable = Table.Table(
        ( '%3d', 'Dup' ),
        ( '%5d', 'Shaft' ),
        ( '%6.2f', 'Az' ),
        ( '%6d', 'Indx A' ),
        ( '%7d', 'Seq # A' ),
        ( '%17.6f', 'IRIG A' ),
        ( '%6d', 'Indx B' ),
        ( '%7d', 'Seq # B' ),
        ( '%8.6f', 'IRIG B-A' ),
        ( '%5d', 'Est C' ),
        ( '%5d', 'Act C' ),
        ( '%5d', 'Diff' ),
        )

    kGapTable = Table.Table(
        ( '%3d', 'Chg' ),
        ( '%5d', 'Shaft' ),
        ( '%6.2f', 'Az' ),
        ( '%6d', 'Index' ),
        ( '%7d', 'Seq #' ),
        ( '%17.6f', 'IRIG B' ),
        ( '%8.6f', 'IRIG B-A' ),
        ( '%5.2f', 'Mult' ),
        )

    kFlag = 'E'

    def __init__( self, expectedDelta = 15, multiple = 0.25 ):
        self.multiple = multiple
        self.expectedDelta = expectedDelta
        self.reset()

    def reset( self ):
        self.maxAllowedVariance = self.expectedDelta * 2
        self.foundDupes = []
        self.foundGaps = []
        self.deltas = []
        self.deltaSum = 0
        self.deltaSumCounter = 0
        self.average = 0.0
        self.firstMessage = None
        self.lastMessage = None

    def fullCheck( self, records ):
        self.reset()
        for pair in zip( records, records[ 1 : ] ):
            self.checkPairs( *pair )
        self.checkAll( records )

    def checkPairs( self, x, y ):
        xShaft = x.getVMEShaftEncoding()
        yShaft = y.getVMEShaftEncoding()

        #
        # If the two message share the same shaft encoding, we record the first
        # and keep track of the the last message with the same value.
        #
        if xShaft == yShaft:
            if self.firstMessage is None:
                self.firstMessage = x
            y.setDupeFlag( self.kFlag )
            self.lastMessage = y
        else:

            #
            # Detect when we end a span of duplicate values
            #
            if self.firstMessage is not None:
                self.foundDupes.append( ( self.firstMessage, self.lastMessage,
                                          yShaft ) )
                self.firstMessage = None
                delta = None

            else:

                #
                # Record the shaft encoding deltas between records for later
                # processing to detect abnormally large gaps in shaft encoding
                # values
                #
                delta = yShaft - xShaft

                #
                # Handle wrap-around when crossing north
                #
                if delta < 0:
                    delta += 65535

                #
                # Only let 'reasonable' values into our average calculation
                #
                if delta <= self.maxAllowedVariance:
                    self.deltaSum += delta
                    self.deltaSumCounter += 1

            self.deltas.append( delta )

    def checkAll( self, records ):

        #
        # Finish any active duplicate span detection.
        #
        if self.firstMessage is not None:
            self.foundDupes.append( ( self.firstMessage, self.lastMessage,
                                      None ) )

        #
        # Cannot continue if we cannot calculate the average shaft encoding
        # delta between messages.
        #
        if self.deltaSumCounter == 0:
            return

        self.average = float( self.deltaSum ) / self.deltaSumCounter

        #
        # Calcuate the new delta filter value based on the average deltas
        #
        self.maxAllowedVariance = self.multiple * self.average

        #
        # Revisit the records to record abnormal delta values
        #
        recordCounter = 0
        for delta in self.deltas:
            if delta is not None:
                variance = abs( delta - self.average )
                if variance > self.maxAllowedVariance:
                    x = records[ recordCounter ]
                    y = records[ recordCounter + 1 ]
                    y.setGapFlag( self.kFlag )
                    self.foundGaps.append( ( x, y, delta,
                                             variance / self.average ) )
            recordCounter += 1

    def printReport( self ):
        print ' Shaft Encoding Anomalies'
        print ' ------------------------'
        print ' Found: %d  Average Step: %f  Max Acceptable Variance: %f  ' \
            '(x%.3f)' % \
            ( len( self.foundDupes ) + len( self.foundGaps ), self.average,
              self.maxAllowedVariance, self.multiple )
        print

        if len( self.foundDupes ):
            self.kDupeTable.setIndent( 2 )
            self.kDupeTable.printHeader()
            for each in self.foundDupes:
                self.emitDupe( *each )
            print

        if len( self.foundGaps ):
            self.kGapTable.setIndent( 2 )
            self.kGapTable.printHeader()
            for each in self.foundGaps:
                self.emitGap( *each )
            print

    def emitDupe( self, x, y, nextShaftEncoding ):
        span = y.getMessageSequence() - x.getMessageSequence()
        estimatedShaftEncoding = x.getVMEShaftEncoding() + ( span + 1 ) * \
            self.average
        if nextShaftEncoding:
            delta = nextShaftEncoding - estimatedShaftEncoding
        else:
            delta = None
        self.kDupeTable.printValues(
            span,
            x.getVMEShaftEncoding(), x.getAzimuth(),
            x.getMessageSequence(), x.getVMESequenceCounter(),
            x.getVMEIRIGTime(),
            y.getMessageSequence(), y.getVMESequenceCounter(),
            y.getVMEIRIGTime() - x.getVMEIRIGTime(),
            estimatedShaftEncoding, nextShaftEncoding, delta )

    def emitGap( self, x, y, gap, multiple ):
        self.kGapTable.printValues(
            gap,
            y.getVMEShaftEncoding(), y.getAzimuth(),
            y.getMessageSequence(),
            y.getVMESequenceCounter(),
            y.getVMEIRIGTime(),
            y.getVMEIRIGTime() - x.getVMEIRIGTime(),
            multiple )

class SequenceCounterChecker:

    kDupeTable = Table.Table(
        ( '%3d', 'Dup' ),
        ( '%7d', 'Seq #' ),
        ( '%6d', 'Indx A' ),
        ( '%17.6f', 'IRIG A' ),
        ( '%5d', 'SE A' ),
        ( '%6.2f', 'Az A' ),
        ( '%6d', 'Indx B' ),
        ( '%8.6f', 'IRIG B-A' ),
        ( '%5d', 'SE B' ),
        ( '%6.2f', 'Az B' ),
        )

    kGapTable = Table.Table(
        ( '%3d', 'Chg' ),
        ( '%6d', 'Indx A' ),
        ( '%5d', 'SE A' ),
        ( '%6.2f', 'Az A' ),
        ( '%6d', 'Indx B' ),
        ( '%7d', 'Seq # B' ),
        ( '%5d', 'SE B' ),
        ( '%6.2f', 'Az B' ),
        ( '%17.6f', 'IRIG B' ),
        ( '%8.6f', 'IRIG B-A' ),
        )

    kFlag = '#'

    def __init__( self ):
        self.reset()

    def reset( self ):
        self.foundDupes = []
        self.foundGaps = []
        self.firstMessage = None
        self.lastMessage = None

    def fullCheck( self, records ):
        self.reset()
        for pair in zip( records, records[ 1 : ] ):
            self.checkPairs( *pair )
        self.checkAll( records )

    def checkPairs( self, x, y ):
        xSeq = x.getVMESequenceCounter()
        ySeq = y.getVMESequenceCounter()

        #
        # If the two message share the same shaft encoding, we record the first
        # and keep track of the the last message with the same value.
        #
        if xSeq == ySeq:
            if self.firstMessage is None:
                self.firstMessage = x
            y.setDupeFlag( self.kFlag )
            self.lastMessage = y
        else:

            #
            # Detect when we end a span of duplicate values
            #
            if self.firstMessage is not None:
                self.foundDupes.append( ( self.firstMessage,
                                          self.lastMessage ) )
                self.firstMessage = None

            else:
                delta = ySeq - xSeq
                if delta != 1:
                    y.setGapFlag( self.kFlag )
                    self.foundGaps.append( ( x, y, delta ) )

    def checkAll( self, records ):

        #
        # Finish any active duplicate span detection.
        #
        if self.firstMessage is not None:
            self.foundDupes.append( ( self.firstMessage, self.lastMessage ) )

    def printReport( self ):
        print ' Sequence Number Anomalies'
        print ' -------------------------'
        print ' Found:', ( len( self.foundDupes ) + len( self.foundGaps ) )
        print
        
        if len( self.foundDupes ):
            self.kDupeTable.setIndent( 2 )
            self.kDupeTable.printHeader()
            for each in self.foundDupes:
                self.emitDupe( *each )
            print

        if len( self.foundGaps ):
            self.kGapTable.setIndent( 2 )
            self.kGapTable.printHeader()
            for each in self.foundGaps:
                self.emitGap( *each )
            print

    def emitDupe( self, x, y ):
        span = y.getMessageSequence() - x.getMessageSequence()
        self.kDupeTable.printValues(
            span,
            x.getVMESequenceCounter,
            x.getMessageSequence(), x.getVMEIRIGTime(),
            x.getVMEShaftEncoding(), x.getAzimuth(),
            y.getMessageSequence(), y.getVMEIRIGTime() - x.getVMEIRIGTime(),
            y.getVMEShaftEncoding(), y.getAzimuth() )

    def emitGap( self, x, y, gap ):
        self.kGapTable.printValues(
            gap,
            x.getMessageSequence(), 
            x.getVMEShaftEncoding(), x.getAzimuth(),
            y.getMessageSequence(), y.getVMESequenceCounter(),
            y.getVMEShaftEncoding(), y.getAzimuth(),
            y.getVMEIRIGTime(), y.getVMEIRIGTime() - x.getVMEIRIGTime() )

class IRIGTimeChecker( object ):

    kDupeTable = Table.Table(
        ( '%3d', 'Dup' ),
        ( '%17.6f', 'IRIG Value' ),
        ( '%6d', 'Indx A' ),
        ( '%7d', 'Seq # A' ),
        ( '%5d', 'ShftA' ),
        ( '%6d', 'Indx B' ),
        ( '%7d', 'Seq # B' ),
        ( '%5d', 'ShftB' ),
        ( '%17.6f', 'Est IRIG C' ),
        ( '%17.6f', 'Act IRIG C' ),
        ( '%8.6f', 'Diff' ),
        )

    kGapTable = Table.Table(
        ( '%9.6f', 'Change' ),
        ( '%6d', 'Indx A' ),
        ( '%7d', 'Seq # A' ),
        ( '%5d', 'ShftA' ),
        ( '%6.2f', 'Az A' ),
        ( '%17.6f', 'IRIG B' ),
        ( '%6d', 'Indx B' ),
        ( '%7d', 'Seq # B' ),
        ( '%5d', 'ShftB' ),
        ( '%6.2f', 'Az B' ),
        ( '%5.2f', 'Mult' ),
        )

    kFlag = 'I'

    def __init__( self, expectedFrequency = 360.0, multiple = 0.25 ):
        self.multiple = multiple
        self.expectedDelta = 1.0 / expectedFrequency
        self.reset()

    def reset( self ):
        self.maxAllowedVariance = self.expectedDelta * 2
        self.foundDupes = []
        self.foundGaps = []
        self.deltas = []
        self.deltaSum = 0.0
        self.deltaSumCounter = 0
        self.average = 0.0
        self.firstMessage = None
        self.lastMessage = None

    def fullCheck( self, records ):
        self.reset()
        for pair in zip( records, records[ 1 : ] ):
            self.checkPairs( *pair )
        self.checkAll( records )

    def checkPairs( self, x, y ):
        xTime = x.getVMEIRIGTime()
        yTime = y.getVMEIRIGTime()

        #
        # If the two message share the same IRIG time, we record the first and
        # keep track of the the last message with the same value.
        #
        if xTime == yTime:
            if self.firstMessage is None:
                self.firstMessage = x
            y.setDupeFlag( self.kFlag )
            self.lastMessage = y
        else:

            #
            # Detect when we end a span of duplicate values
            #
            if self.firstMessage is not None:
                self.foundDupes.append( ( self.firstMessage, self.lastMessage,
                                          yTime ) )
                self.firstMessage = None
                delta = None
            else:

                #
                # Only let 'reasonable' values into our average calculation
                #
                delta = yTime - xTime
                if delta > 0 and delta < self.maxAllowedVariance:
                    deltaSum += delta
                    deltaSumCounter += 1

            deltas.append( delta )

    def checkAll( self, records ):

        #
        # Finish any active duplicate span detection.
        #
        if self.firstMessage is not None:
            self.foundDupes.append( ( self.firstMessage, self.lastMessage,
                                      None ) )

        #
        # Cannot continue if we cannot calcuate the average IRIG change between
        # messages.
        #
        if self.deltaSumCounter == 0:
            return

        self.average = float( deltaSum ) / self.deltaSumCounter

        #
        # Calculate the gap filter value based on the average deltas
        #
        self.maxAllowedVariance = self.multiple * self.average

        #
        # Revisit the records to record the abnormal delta values
        #
        recordCounter = 0
        for delta in self.deltas:
            if delta is not None:
                variance = abs( delta - self.average )
                if delta < 0.0 or variance > self.maxAllowedVariance:
                    x = records[ recordCounter ]
                    y = records[ recordCounter + 1 ]
                    y.setGapFlag( self.kFlag )
                    self.foundGaps.append( ( x, y, delta,
                                             variance / self.average ) )
            recordCounter += 1

    def printReport( self ):
        print ' IRIG TimeStamp Anomalies'
        print ' ------------------------'
        freq = 0.0
        if self.average:
            freq = 1.0 / self.average
        print ' Found: %d  Average Step: %f (%.2f Hz)  ' \
            'Max Acceptable Variance: %f (x%.3f)' % \
            ( len( self.foundDupes ) + len( self.foundGaps ),
              self.average, freq, self.maxAllowedVariance, self.multiple )
        print

        if len( self.foundDupes ):
            self.kDupeTable.setIndent( 2 )
            self.kDupeTable.printHeader()
            for each in self.foundDupes:
                self.emitDupe( *each )
            print

        if len( self.foundGaps ):
            self.kGapTable.setIndent( 2 )
            self.kGapTable.printHeader()
            for each in foundGaps:
                self.emitGap( *each )
            print

    def emitDupe( self, x, y, nextIRIGTime ):
        span = y.getMessageSequence() - x.getMessageSequence()
        estimatedIRIGTime = x.getVMEIRIGTime() + ( span + 1 ) * \
            self.average
        if nextIRIGTime:
            delta = nextIRIGTime - estimatedIRIGTime
        else:
            delta = None
        self.kDupeTable.printValues(
            span,
            x.getVMEIRIGTime(),
            x.getMessageSequence(), x.getVMESequenceCounter(),
            x.getVMEShaftEncoding(),
            y.getMessageSequence(), y.getVMESequenceCounter(),
            y.getVMEShaftEncoding(),
            estimatedIRIGTime, nextIRIGTime, delta )

    def emitGap( self, x, y, gap, multiple ):
        self.kGapTable.printValues(
            gap,
            x.getMessageSequence(),
            x.getVMESequenceCounter(),
            x.getVMEShaftEncoding(),
            x.getAzimuth(),
            y.getVMEIRIGTime(),
            y.getMessageSequence(),
            y.getVMESequenceCounter(),
            y.getVMEShaftEncoding(),
            y.getAzimuth(),
            multiple )

class SampleCountChecker( object ):

    kChangeTable = Table.Table(
        ( '%5d', 'Chg' ),
        ( '%4d', 'Size' ),
        ( '%6d', 'Indx B' ),
        ( '%7d', 'Seq # B' ),
        ( '%5d', 'ShftB' ),
        ( '%6.2f', 'Az B' ),
        ( '%17.6f', 'IRIG B' ),
        ( '%8.6f', 'IRIG B-A' ),
        )

    kFlag = 'Z'

    def __init__( self, maxAllowedChange = 4 ):
        self.maxAllowedChange = maxAllowedChange
        self.reset()

    def reset( self ):
        self.foundChanges = []
        self.sizeSum = 0
        self.average = 0.0
        self.minSeen = None
        self.maxSeen = None

    def fullCheck( self, records ):
        self.reset()
        for x in records:
            self.sizeSum += x.getSampleCount()
        self.checkAll( records )

    def checkPairs( self, x, y ):
        xSize = x.getSampleCount()
        ySize = y.getSampleCount()
        if self.sizeSum == 0:
            self.sizeSum = xSize + ySize
            self.minSeen = min( xSize, ySize )
            self.maxSeen = max( xSize, ySize )
        else:
            self.sizeSum += ySize
            if ySize < self.minSeen:
                self.minSeen = ySize
            elif ySize > self.maxSeen:
                self.maxSeen = ySize
        variance = abs( ySize - xSize )
        if variance > self.maxAllowedChange:
            y.setFlag( self.kFlag )
            self.foundChanges.append( ( x, y, ySize - xSize ) )

    def checkAll( self, records ):
        if len( records ) == 0:
            return
        self.average = float( self.sizeSum ) / len( records )

    def printReport( self ):
        print ' Sample Count Anomalies'
        print ' ----------------------'
        print ' Found: %d  Average: %.2f  Max Acceptable Change: %d' % \
            ( len( self.foundChanges ), self.average, self.maxAllowedChange )
        print

        if len( self.foundChanges ):
            self.kChangeTable.setIndent( 2 )
            self.kChaneTable.printHeader()
            for each in self.foundChanges:
                self.emitChane( *each )
            print

    def emitChange( self, x, y, change ):
        self.kChangeTable.printValues(
            change,
            y.getSampleCount(),
            y.getMessageSequence(), y.getVMESequenceCounter(),
            y.getVMEShaftEncoding(), y.getAzimuth(),
            y.getMessageSequence(), y.getVMESequenceCounter(),
            y.getVMEIRIGTime(), y.getVMEIRIGTime() - x.getVMEIRIGTime()
            )

def loader( path, start = 0, count = -1 ):
    records = Records()
    records.load( open( path, 'rb' ), start, count )
    return records

def check( pattern, frequency ):

    flaggedTable = Table.Table( ( '%6d', 'Index' ),
                          ( '%7d', 'Seq #' ),
                          ( '%5d', 'Shaft' ),
                          ( '%6.2f', 'Az' ),
                          ( '%17.6f', 'IRIG' ),
                          ( '%12.6f', 'Offset' ),
                          ( '%2s', SequenceCounterChecker.kFlag ),
                          ( '%2s', ShaftEncodingChecker.kFlag ),
                          ( '%2s', IRIGTimeChecker.kFlag ),
                          ( '%1s', SampleCountChecker.kFlag ),
                          )

    checkers = (
        SequenceCounterChecker(),
        ShaftEncodingChecker(),
        IRIGTimeChecker( frequency ),
        SampleCountChecker(),
        )

    for priPath in sorted( glob.glob( pattern ) ):

        print '...processing', priPath

        reportPath = os.path.extsep.join(
            [ os.path.splitext( priPath )[ 0 ], 'priinfo' ] )

        records = Records()
        records.setCheckers( checkers )
        for checker in checkers:
            checker.reset()

        records.load( open( priPath, 'rb' ) )
        if records.messageType != 2:
            print '-- invalid message type -', records.messageType
            continue

        sys.stdout = file( reportPath, 'w' )
        print 'Record Count: %17d' % len( records )
        if len( records ) == 0:
            sys.stdout = sys.__stdout__
            continue

        flagged = []
        for record in records:
            flags = record.getFlags()
            if flags is not None:
                flagged.append( record )

        startTime = records[ 0 ].getVMEIRIGTime()
        endTime = records[ -1 ].getVMEIRIGTime()
        print '  Start Time: %17.6f' % startTime
        print '    End Time: %17.6f' % endTime
        print '    Duration: %17.3f' % ( endTime - startTime )
        print '   Anomalies: %17d' % len( flagged )

        if len( flagged ):
            print
            print '#: sequence counter  I: IRIG time  E: shaft encoding  ' \
                'Z: sample size'
            print '-: gap/drop  +: duplicate value'
            print

            flaggedTable.printHeader()
            for record in flagged:
                flags = record.getFlags()
                flaggedTable.printValues(
                    record.getMessageSequence(),
                    record.getVMESequenceCounter(),
                    record.getVMEShaftEncoding(),
                    record.getAzimuth(),
                    record.getVMEIRIGTime(),
                    record.getVMEIRIGTime() - startTime,
                    record.getFlag( SequenceCounterChecker.kFlag ),
                    record.getFlag( ShaftEncodingChecker.kFlag ),
                    record.getFlag( IRIGTimeChecker.kFlag ),
                    record.getFlag( SampleCountChecker.kFlag ),
                    )
            print

        for checker in checkers:
            checker.printReport()

        sys.stdout = sys.__stdout__
