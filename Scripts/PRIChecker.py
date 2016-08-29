#!/usr/bin/env python

import Table

class PRIChecker( object ):

    def check( self, data ):
        self.reset()
        for pair in zip( data, data[ 1 : ] ):
            self.checkPairs( data, *pair )
        self.checkAll( data )

class SequenceCounterChecker( PRIChecker ):

    kDupeTable = Table.Table(
        2,
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
        2,
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

    def __repr__( self ): return '<SequenceCounterChecker>'
    def __hash__( self ): return hash( repr( self ) )
    def __cmp__( self, other ): return cmp( repr( self ), repr( other ) )

    def reset( self ):
        self.foundDupes = []
        self.foundGaps = []
        self.firstMessage = None
        self.lastMessage = None

    def checkPairs( self, data, x, y ):
        xSeq = x.getVMESequenceCounter()
        ySeq = y.getVMESequenceCounter()
        if xSeq == ySeq:
            if self.firstMessage is None:
                self.firstMessage = x
            y.setDupeFlag( self.kFlag )
            data.foundAnomaly( y )
            self.lastMessage = y
        else:
            if self.firstMessage is not None:
                self.foundDupes.append( ( self.firstMessage,
                                          self.lastMessage ) )
                self.firstMessage = None
            else:
                delta = ySeq - xSeq
                if delta != 1:
                    y.setGapFlag( self.kFlag )
                    data.foundAnomaly( y )
                    self.foundGaps.append( ( x, y, delta ) )

    def checkAll( self, data ):
        if self.firstMessage is not None:
            self.foundDupes.append( ( self.firstMessage, self.lastMessage ) )

    def printReport( self, fd ):
        fd.write( ' Sequence Counter Anomalies\n' )
        fd.write( ' --------------------------\n' )
        fd.write( ' Found: %d\n\n' % ( len( self.foundDupes ) +
                                       len( self.foundGaps ) ) )

        if len( self.foundDupes ):
            self.kDupeTable.printHeader( fd )
            for each in self.foundDupes:
                self.emitDupe( fd, *each )
            fd.write( '\n' )

        if len( self.foundGaps ):
            self.kGapTable.printHeader( fd )
            for each in self.foundGaps:
                self.emitGap( fd, *each )
            fd.write( '\n' )

    def emitDupe( self, fd, x, y ):
        span = y.getMessageSequence() - x.getMessageSequence()
        self.kDupeTable.printValues(
            fd,
            span,
            x.getVMESequenceCounter,
            x.getMessageSequence(), x.getVMEIRIGTime(),
            x.getVMEShaftEncoding(), x.getAzimuth(),
            y.getMessageSequence(), y.getVMEIRIGTime() - x.getVMEIRIGTime(),
            y.getVMEShaftEncoding(), y.getAzimuth() )

    def emitGap( self, fd, x, y, gap ):
        self.kGapTable.printValues(
            fd,
            gap,
            x.getMessageSequence(), 
            x.getVMEShaftEncoding(), x.getAzimuth(),
            y.getMessageSequence(), y.getVMESequenceCounter(),
            y.getVMEShaftEncoding(), y.getAzimuth(),
            y.getVMEIRIGTime(), y.getVMEIRIGTime() - x.getVMEIRIGTime() )

class ShaftEncodingChecker( PRIChecker ):

    kDupeTable = Table.Table(
        2,
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
        2,
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

    def __init__( self, multiple = 0.25 ):
        self.multiple = multiple
        self.reset()

    def __repr__( self ): return '<ShaftEncodingChecker>'
    def __hash__( self ): return hash( repr( self ) )
    def __cmp__( self, other ): return cmp( repr( self ), repr( other ) )

    def reset( self ):
        self.foundDupes = []
        self.foundGaps = []
        self.deltas = []
        self.median = 0
        self.firstMessage = None
        self.lastMessage = None

    def checkPairs( self, data, x, y ):
        xShaft = x.getVMEShaftEncoding()
        yShaft = y.getVMEShaftEncoding()
        if xShaft == yShaft:
            if self.firstMessage is None:
                self.firstMessage = x
            y.setDupeFlag( self.kFlag )
            data.foundAnomaly( y )
            self.lastMessage = y
        else:
            if self.firstMessage is not None:
                self.foundDupes.append( ( self.firstMessage, self.lastMessage,
                                          yShaft ) )
                self.firstMessage = None
                delta = None

            else:
                delta = yShaft - xShaft
                if delta < 0:
                    delta += 65536
            self.deltas.append( ( delta, x, y ) )

    def checkAll( self, data ):
        if self.firstMessage is not None:
            self.foundDupes.append( ( self.firstMessage, self.lastMessage,
                                      None ) )
        if len( self.deltas ) == 0:
            return

        orderedDeltas = [ x[ 0 ] for x in self.deltas ]
        orderedDeltas.sort()
        self.median = orderedDeltas[ len( orderedDeltas ) / 2 ]
        self.maxAllowedVariance = self.multiple * self.median
        for delta, x, y in self.deltas:
            if delta is not None:
                variance = abs( delta - self.median )
                if variance > self.maxAllowedVariance:
                    y.setGapFlag( self.kFlag )
                    data.foundAnomaly( y )
                    self.foundGaps.append( ( x, y, delta,
                                             variance / self.median ) )

    def printReport( self, fd ):
        fd.write( ' Shaft Encoding Anomalies\n' )
        fd.write( ' ------------------------\n' )
        fd.write( ' Found: %d  Median Step: %d  Max Acceptable '
                  'Variance: %f (x%.3f)\n\n' %
                  ( len( self.foundDupes ) + len( self.foundGaps ),
                    self.median, self.maxAllowedVariance, self.multiple ) )

        if len( self.foundDupes ):
            self.kDupeTable.printHeader( fd )
            for each in self.foundDupes:
                self.emitDupe( fd, *each )
            fd.write( '\n' )

        if len( self.foundGaps ):
            self.kGapTable.printHeader( fd )
            for each in self.foundGaps:
                self.emitGap( fd, *each )
            fd.write( '\n' )

    def emitDupe( self, fd, x, y, nextShaftEncoding ):
        span = y.getMessageSequence() - x.getMessageSequence()
        estimatedShaftEncoding = x.getVMEShaftEncoding() + ( span + 1 ) * \
            self.median
        if nextShaftEncoding:
            delta = nextShaftEncoding - estimatedShaftEncoding
        else:
            delta = None
        self.kDupeTable.printValues(
            fd,
            span,
            x.getVMEShaftEncoding(), x.getAzimuth(),
            x.getMessageSequence(), x.getVMESequenceCounter(),
            x.getVMEIRIGTime(),
            y.getMessageSequence(), y.getVMESequenceCounter(),
            y.getVMEIRIGTime() - x.getVMEIRIGTime(),
            estimatedShaftEncoding, nextShaftEncoding, delta )

    def emitGap( self, fd, x, y, gap, multiple ):
        self.kGapTable.printValues(
            fd, 
            gap,
            y.getVMEShaftEncoding(), y.getAzimuth(),
            y.getMessageSequence(),
            y.getVMESequenceCounter(),
            y.getVMEIRIGTime(),
            y.getVMEIRIGTime() - x.getVMEIRIGTime(),
            multiple )

class HighFreqencyCounterChecker( PRIChecker ):

    kDupeTable = Table.Table(
        2,
        ( '%3d', 'Dup' ),
        ( '%10d', 'Value' ),
        ( '%6d', 'Indx A' ),
        ( '%7d', 'Seq # A' ),
        ( '%17.6f', 'IRIG A' ),
        ( '%6d', 'Indx B' ),
        ( '%7d', 'Seq # B' ),
        ( '%8.6f', 'IRIG B-A' ),
        ( '%10d', 'Est C' ),
        ( '%10d', 'Act C' ),
        ( '%8d', 'Diff' ),
        )

    kGapTable = Table.Table(
        2,
        ( '%5d', 'Devia' ),
        ( '%6d', 'Indx A' ),
        ( '%7d', 'Seq # A' ),
        ( '%17.6f', 'IRIG A' ),
        ( '%10d', 'Counter A' ),
        ( '%6d', 'Indx B' ),
        ( '%7d', 'Seq # B' ),
        ( '%17.6f', 'IRIG B' ),
        ( '%10d', 'Counter B' ),
        )

    kFlag = '8'

    def __init__( self, maxAllowedChange = 50 ):
        self.maxAllowedChange = maxAllowedChange
        self.reset()

    def __repr__( self ): return '<HighFrequencyCounterChecker>'
    def __hash__( self ): return hash( repr( self ) )
    def __cmp__( self, other ): return cmp( repr( self ), repr( other ) )

    def reset( self ):
        self.foundDupes = []
        self.foundGaps = []
        self.deltas = []
        self.median = 0
        self.firstMessage = None
        self.lastMessage = None
        self.maxVariation = 0
        self.minVariation = 0

    def checkPairs( self, data, x, y ):
        xValue = x.getVMETimeStamp()
        yValue = y.getVMETimeStamp()
        if xValue == yValue:
            if self.firstMessage is None:
                self.firstMessage = x
            y.setDupeFlag( self.kFlag )
            data.foundAnomaly( y )
            self.lastMessage = y
        else:
            if self.firstMessage is not None:
                self.foundDupes.append( ( self.firstMessage, self.lastMessage,
                                          yValue ) )
                self.firstMessage = None
                delta = None
            else:
                delta = yValue - xValue
                if delta < 0:
                    delta += 65536 * 65536
            self.deltas.append( ( delta, x, y ) )

    def checkAll( self, data ):
        if self.firstMessage is not None:
            self.foundDupes.append( ( self.firstMessage, self.lastMessage,
                                      None ) )

        if len( self.deltas ) == 0:
            return

        orderedDeltas = [ x[ 0 ] for x in self.deltas ]
        orderedDeltas.sort()
        self.median = orderedDeltas[ len( orderedDeltas ) / 2 ]
        for delta, x, y in self.deltas:
            if delta is not None:
                variance = delta - self.median
                if variance < self.minVariation:
                    self.minVariation = variance
                elif variance > self.maxVariation:
                    self.maxVariation = variance
                if abs( variance ) > self.maxAllowedChange:
                    y.setGapFlag( self.kFlag )
                    data.foundAnomaly( y )
                    self.foundGaps.append( ( x, y, variance ) )

    def printReport( self, fd ):
        fd.write( ' High Frequency Counter Anomalies\n' )
        fd.write( ' --------------------------------\n' )
        fd.write( ' Found: %d  Median Change: %d  Max Acceptable '
                  'Variation: +/- %d  Min: %d Max: %d\n\n' %
                  ( len( self.foundDupes ) + len( self.foundGaps ),
                    self.median, self.maxAllowedChange,
                    self.minVariation, self.maxVariation ) )

        if len( self.foundDupes ):
            self.kDupeTable.printHeader( fd )
            for each in self.foundDupes:
                self.emitDupe( fd, *each )
            fd.write( '\n' )

        if len( self.foundGaps ):
            self.kGapTable.printHeader( fd )
            for each in self.foundGaps:
                self.emitGap( fd, *each )
            fd.write( '\n' )

    def emitDupe( self, fd, x, y, nextTimeStamp ):
        span = y.getMessageSequence() - x.getMessageSequence()
        estimatedTimeStamp = x.getVMETimeStamp() + ( span + 1 ) * self.median
        if nextTimeStamp:
            delta = nextTimeStamp - estimatedTimeStamp
        else:
            delta = None
        self.kDupeTable.printValues(
            fd,
            span,
            x.getVMETimeStamp(),
            x.getMessageSequence(), x.getVMESequenceCounter(),
            x.getVMEIRIGTime(),
            y.getMessageSequence(), y.getVMESequenceCounter(),
            y.getVMEIRIGTime(),
            y.getVMEIRIGTime() - x.getVMEIRIGTime(),
            estimatedTimeStamp, nextTimeStamp, delta )

    def emitGap( self, fd, x, y, variance ):
        self.kGapTable.printValues(
            fd, 
            variance,
            x.getMessageSequence(), x.getVMESequenceCounter(),
            x.getVMEIRIGTime(),
            x.getVMETimeStamp(),
            y.getMessageSequence(), y.getVMESequenceCounter(),
            y.getVMEIRIGTime(),
            y.getVMETimeStamp() )

class IRIGTimeChecker( PRIChecker ):

    kDupeTable = Table.Table(
        2,
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
        2,
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

    def __repr__( self ): return '<IRIGTimeChecker>'
    def __hash__( self ): return hash( repr( self ) )
    def __cmp__( self, other ): return cmp( repr( self ), repr( other ) )

    def reset( self ):
        self.maxAllowedVariance = self.expectedDelta * 2
        self.foundDupes = []
        self.foundGaps = []
        self.deltas = []
        self.median = 0.0
        self.firstMessage = None
        self.lastMessage = None

    def checkPairs( self, data, x, y ):
        xTime = x.getVMEIRIGTime()
        yTime = y.getVMEIRIGTime()
        if xTime == yTime:
            if self.firstMessage is None:
                self.firstMessage = x
            y.setDupeFlag( self.kFlag )
            data.foundAnomaly( y )
            self.lastMessage = y
        else:
            if self.firstMessage is not None:
                self.foundDupes.append( ( self.firstMessage, self.lastMessage,
                                          yTime ) )
                self.firstMessage = None
                delta = 0.0
            else:
                delta = yTime - xTime
            self.deltas.append( ( delta, x, y ) )

    def checkAll( self, data ):
        if self.firstMessage is not None:
            self.foundDupes.append( ( self.firstMessage, self.lastMessage,
                                      None ) )

        if len( self.deltas ) == 0:
            return

        orderedDeltas = [ x[ 0 ] for x in self.deltas ]
        orderedDeltas.sort()
        self.median = orderedDeltas[ len( orderedDeltas ) / 2 ]
        self.maxAllowedVariance = self.multiple * self.median
        for delta, x, y in self.deltas:
            if delta is not 0.0:
                variance = abs( delta - self.median )
                if delta < 0.0 or variance > self.maxAllowedVariance:
                    y.setGapFlag( self.kFlag )
                    data.foundAnomaly( y )
                    self.foundGaps.append( ( x, y, delta,
                                             variance / self.median ) )

    def printReport( self, fd ):
        fd.write( ' IRIG TimeStamp Anomalies\n' )
        fd.write( ' ------------------------\n' )
        freq = 0.0
        if self.median:
            freq = 1.0 / self.median
        fd.write( ' Found: %d  Median Step: %f (%.2f Hz) Max Acceptable '
                  'Variance: %f (x%.3f)\n\n' % 
                  ( len( self.foundDupes ) + len( self.foundGaps ),
                    self.median, freq, self.maxAllowedVariance,
                    self.multiple ) )

        if len( self.foundDupes ):
            self.kDupeTable.printHeader( fd )
            for each in self.foundDupes:
                self.emitDupe( fd, *each )
            fd.write( '\n' )

        if len( self.foundGaps ):
            self.kGapTable.printHeader( fd )
            for each in self.foundGaps:
                self.emitGap( fd, *each )
            fd.write( '\n' )

    def emitDupe( self, fd, x, y, nextIRIGTime ):
        span = y.getMessageSequence() - x.getMessageSequence()
        estimatedIRIGTime = x.getVMEIRIGTime() + ( span + 1 ) * \
            self.median
        if nextIRIGTime:
            delta = nextIRIGTime - estimatedIRIGTime
        else:
            delta = None
        self.kDupeTable.printValues(
            fd,
            span,
            x.getVMEIRIGTime(),
            x.getMessageSequence(), x.getVMESequenceCounter(),
            x.getVMEShaftEncoding(),
            y.getMessageSequence(), y.getVMESequenceCounter(),
            y.getVMEShaftEncoding(),
            estimatedIRIGTime, nextIRIGTime, delta )

    def emitGap( self, fd, x, y, gap, multiple ):
        self.kGapTable.printValues(
            fd,
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

class SampleCountChecker( PRIChecker ):

    kChangeTable = Table.Table(
        2,
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

    def __init__( self,  maxAllowedChange = 4 ):
        self.maxAllowedChange = maxAllowedChange
        self.reset()

    def __repr__( self ): return '<SampleCountChecker>'
    def __hash__( self ): return hash( repr( self ) )
    def __cmp__( self, other ): return cmp( repr( self ), repr( other ) )

    def reset( self ):
        self.foundChanges = []
        self.sizeSum = 0
        self.median = 0.0
        self.minSeen = None
        self.maxSeen = None

    def checkPairs( self, data, x, y ):
        xSize = x.getSampleCount()
        ySize = y.getSampleCount()
        if self.sizeSum == 0:
            self.minSeen = min( xSize, ySize )
            self.maxSeen = max( xSize, ySize )
        else:
            if ySize < self.minSeen:
                self.minSeen = ySize
            elif ySize > self.maxSeen:
                self.maxSeen = ySize

    def checkAll( self, data ):
        if len( data ) == 0:
            return

        orderedValues = [ x.getSampleCount() for x in data ]
        orderedValues.sort()
        self.median = orderedValues[ len( data ) / 2 ]

        x = None
        for y in data:
            variance = abs( len( y ) - self.median )
            if variance > self.maxAllowedChange:
                y.setFlag( self.kFlag )
                data.foundAnomaly( y )
                self.foundChanges.append( ( x, y, variance ) )
            x = y

    def printReport( self, fd ):
        fd.write( ' Sample Count Anomalies\n' )
        fd.write( ' ----------------------\n' )
        fd.write( ' Found: %d  Median: %d  Max Acceptable Change: +/- %d\n\n' % 
                  ( len( self.foundChanges ), self.median,
                    self.maxAllowedChange ) )

        if len( self.foundChanges ):
            self.kChangeTable.printHeader( fd )
            for each in self.foundChanges:
                self.emitChange( fd, *each )
            print

    def emitChange( self, fd, x, y, variance ):
        if x is None:
            xIRIG = 0
        else:
            xIRIG = x.getVMEIRIGTime()
        self.kChangeTable.printValues(
            fd,
            variance,
            y.getSampleCount(),
            y.getMessageSequence(), y.getVMESequenceCounter(),
            y.getVMEShaftEncoding(), y.getAzimuth(),
            y.getVMEIRIGTime(), y.getVMEIRIGTime() - xIRIG
            )
