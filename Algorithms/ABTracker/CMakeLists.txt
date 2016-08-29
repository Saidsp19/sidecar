# -*- Mode: CMake -*-
# 
# CMake build file for the ABTracker algorithm
# 

# Production specification for ABTracker algorithm
#
add_algorithm(ABTracker 
			  ABTracker.cc
			  Track.cc 
	  		  UnitVector.cc 
	      	  Vector.cc)

# Unit tests for ABTracker and its classes
#
add_unit_test(UnitVectorTest.cc UnitVector.cc Vector.cc Utils)
add_unit_test(VectorTest.cc UnitVector.cc Vector.cc Utils)
add_unit_test(ABTrackerTest.cc ABTracker)
