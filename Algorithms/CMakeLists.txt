# -*- Mode: CMake -*-
# 
# CMake build file for the Algorithm library and algorithm DLLs
#

# The Controller class uses QtXML, and the Algorithm class uses VSIPL++. Supply FFTW3 just in case an algorithm
# wishes to use it too (though it should be available directly from VSIPL++)
#

include_directories(${FFTW3_INCLUDE_DIRS}
                    ${QT_INCLUDES}
                    ${MPI_INCLUDE_PATH}
                    ${VSIPL_INCLUDE_DIRS} )

# Production specification for libAlgorithm
#
add_library(Algorithm SHARED 
            Algorithm.cc 
	        ChannelBuffer.cc 
            Controller.cc
	        ControllerStatus.cc 
	        CPIAlgorithm.cc 
	        ManyInAlgorithm.cc 
	        ManyInCPIAlgorithm.cc 
	        ProcessingStat.cc 
	        Recorder.cc 
	        RemoteControllerBase.cc
	        ShutdownMonitor.cc 
	        Utils.cc)

# Linkage dependencies for runner
#
target_link_libraries(Algorithm IO ${VSIPL_LIBRARIES} ${QT_LIBRARIES} ${MPI_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS Algorithm LIBRARY DESTINATION lib)

# Unit tests for libAlgorithm classes
#
add_unit_test(AlgorithmTests.cc Algorithm)
add_unit_test(PastBufferTests.cc Algorithm)
add_unit_test(SynchronizedBufferTests.cc Algorithm)

# Directories to process containing algorithms
#
add_directories(ABTracker
				BinaryOp
				BugCollector
				cfar
				CIntegrate
				Clamp
				ClutterMap
				CPIIntegrate
				CPIMarker
				CPISimpleOp
				Decimator
				Delay
				Despeckle
				Difference
				DynamicThreshold
				EdgeDetector
				extract
				extractWithCentroiding
				FanIn
				FanOut
				filter
				GeoFilter
				inverter
				iqfilter
				LowPassFilter
                #				 MatchedFilter
				mofn
				MTI2
				ncintegrate
				noop
				offset
				oscfar
				Printer
				PRISegmentation
				RangeDopplerMap
				RawPRI
				Recorder
				RGBConverter
				Router
				SandBox
				scale
				ScanCorrelator
				Segment2Extraction
				SegmentConnector
				SegmentSplitter
				SegmentStats
				ShiftBins
				SimpleNOOP
				SimpleOp
				SlidingOp
				SpreadMofN
				Sum
				Summer
				SumNDiff
				Threshold
                #				 TrackInitiator
                #				 TrackMaintainer
				Trimmer
				TSPI
                #				 VideoInterpolation
				Volts2Power
				WindowedMinMax
                )
