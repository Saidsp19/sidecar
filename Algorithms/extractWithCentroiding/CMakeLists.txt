
add_algorithm( ExtractWithCentroiding 
	       	   Centroid.cc
	       	   ExtractWithCentroiding.cc 
	       	   ImageSegmentation.cc 
	       	   Row.cc
	       	   SegmentedTargetImage.cc
	       	   TargetSize.cc )

target_link_libraries( ExtractWithCentroiding )
