
add_algorithm( Volts2Power Volts2Power.cc )

target_link_libraries( Volts2Power )

add_unit_test( Volts2PowerTest.cc Volts2Power )
