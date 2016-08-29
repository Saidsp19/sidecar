#!/usr/bin/env python
#
# Python script that runs extinfo in recording directories for one or more
# blob extraction files.
#
# Usage: extcsv PATTERN DIR [DIR...]
#
# Ex: extcsv *Extract.pri /space1/recordings/20091010*
#

import glob, Logger, os, subprocess, sys, threading

## Derivation of threading.Thread class that invokes extinfo as a separate
# process to convert an extractions PRI file into a CSV file.
#
class GenerateCSV( threading.Thread ):
    
    ## Constructor
    # \param processor the Processor object that is managing us
    # \param path the location of the file to process
    #
    def __init__( self, processor, path ):
        threading.Thread.__init__( self )
        self.processor = processor
        self.path = path
        self.count = 0

    ## Override of threading.Thread method. Spawns a new process that runs the
    # 'extinfo' program to convert Messages::Extractions messages into CVS
    # rows. Invokes Processor.addFinished when done.
    #
    def run( self ):

        #
        # Create a new file with a '.csv' extension
        #
        outputPath = os.path.splitext( self.path )[ 0 ] + '.csv'
        try:
            fd = open( outputPath, 'w' )
            process = subprocess.Popen( [ 'extinfo', '-d', '-c', self.path ], 
                                        stdout = fd, 
                                        stderr = open( '/dev/null', 'w' ) )
            status = os.waitpid( process.pid, 0 )
            fd.flush()
            fd.close()
            self.count = len( open( outputPath, 'r' ).readlines() )

        finally:
            self.processor.addFinished( self )

## Work processor. Creates a queue of pending jobs for the files found on the
# system. Manages a set of GenerateCSV objects so that no more than maxActive
# are running at any one time.
#
class Processor:

    
    ## Constructor. Locates files to process and creates GenerateCSV objects
    # for each one, placing them in the internal pendingQueue.
    # \param pattern the filename patter to locate
    # \param directories one or more directories to process
    # \param maxActive number of active GenerateCSV objects
    #
    def __init__( self, pattern, directories, maxActive ):
        self.pattern = pattern
        self.pendingQueue = []
        self.activeQueue = []
        self.finishedQueue = []
        self.condition = threading.Condition()
        self.maxActive = maxActive
        for directory in directories:
            if os.path.exists( directory ):
                print '... processing directory', directory
                self.addDirectory( directory )
            else:
                print '*** directory', directory, 'does not exist'

    ## Look for matching files in a particular directory on the system. For
    # each file found, create a GenerateCSV object and place in the pending
    # queue.
    # \param directory the directory to process
    #
    def addDirectory( self, directory ):
        here = os.getcwd()
        os.chdir( directory )
        files = glob.glob( self.pattern )
        if len( files ) == 0:
            print '*** no files found matching pattern', self.pattern
        else:
            for file in files:
                print '... procesing file', file
                self.pendingQueue.append( 
                    GenerateCSV( self, os.path.abspath( file ) ) )
        os.chdir( here )

    ## Remove a GenerateCSV object from the activeQueue and place in the
    # finishedQueue and signal the Processor object that the GenerateCSV object
    # is finished.
    # \param obj the GenerateCSV object to work with
    #
    def addFinished( self, obj ):
        self.condition.acquire()
        self.activeQueue.remove( obj )
        self.finishedQueue.append( obj )
        self.condition.notify()
        self.condition.release()

    ## Process the GenerateCSV objects in pendingQueue. When all are done,
    # print out a summary of the work that took place.
    #
    def run( self ):

        #
        # Spawn threads to process the files until there are no more pending
        # requests.
        #
        maxCount = len( self.pendingQueue )
        while len( self.pendingQueue ) > 0:

            #
            # Limit ourselves to maxActive active processes
            #
            self.condition.acquire()
            while len( self.activeQueue ) == self.maxActive:
                self.condition.wait()

            #
            # Start another process
            #
            obj = self.pendingQueue.pop()
            self.activeQueue.append( obj )
            self.condition.release()
            obj.start()

        #
        # There are no more elements in the pending queue. Wait until we have
        # processed all of them.
        #
        self.condition.acquire()
        while len( self.finishedQueue ) != maxCount:
            self.condition.wait()

        #
        # All done! Print out the results.
        #
        for obj in self.finishedQueue:
            print 'wrote', obj.count, 'lines in', obj.path

if __name__ == '__main__':
    if len( sys.argv ) < 3:
        print '*** missing arguments ***'
        print 'Usage: extcsv PATTERN DIR [DIR...]'
        sys.exit( 1 )
    processor = Processor( sys.argv[ 1 ], sys.argv[ 2: ], 8 )
    processor.run()

