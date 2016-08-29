
add_algorithm( SimpleNOOP SimpleNOOP.cc )

target_link_libraries( SimpleNOOP )

add_unit_test( SimpleNOOPTest.cc SimpleNOOP )
