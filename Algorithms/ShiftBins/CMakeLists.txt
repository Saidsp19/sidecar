
add_algorithm( ShiftBins ShiftBins.cc )

target_link_libraries( ShiftBins )

add_unit_test( ShiftBinsTest.cc ShiftBins )
