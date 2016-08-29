
add_algorithm( IQFilter IQFilter.cc )

target_link_libraries( IQFilter )

add_unit_test( IQFilterTest.cc IQFilter )
