
add_algorithm( WindowedMinMax WindowedMinMax.cc )

target_link_libraries( WindowedMinMax )

add_unit_test( WindowedMinMaxTest.cc WindowedMinMax )
