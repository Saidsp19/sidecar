
add_algorithm( VideoInterpolation VideoInterpolation.cc )

target_link_libraries( VideoInterpolation )

add_unit_test( VideoInterpolationTest.cc VideoInterpolation )
