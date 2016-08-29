
ADD_ALGORITHM( NCIntegrate NCIntegrate.cc )

TARGET_LINK_LIBRARIES( NCIntegrate ${VSIPL_LIBRARIES} )

add_unit_test( NCIntegrateTest.cc NCIntegrate )
