
add_algorithm( SimpleOp SimpleOp.cc )

target_link_libraries( SimpleOp )

add_unit_test( SimpleOpTest.cc SimpleOp )
