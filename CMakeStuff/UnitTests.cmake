# -*- Mode: CMake -*-
#
# Macro add_unit_test NAME SOURCE
#
# Create appropriate build rules for an algorithm. NAME is the name of the unit test to make.
#
macro(ADD_UNIT_TEST FILE)

    # Create the output directory if necessary
    #
    if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}")

    # Build the unit test executable.
    #
    get_filename_component(aut_NAME ${FILE} NAME_WE)
    set(aut_NAME "${aut_NAME}")

    set(aut_SRCS ${FILE})
    set(aut_LIBS)

    foreach(aut_FILE ${ARGN})
        get_filename_component(aut_EXT ${aut_FILE} EXT)
        if("${aut_EXT}" STREQUAL ".cc")
            list(APPEND aut_SRCS ${aut_FILE})
        elseif("${aut_EXT}" STREQUAL ".cpp")
            list(APPEND aut_SRCS ${aut_FILE})
        else("${aut_EXT}" STREQUAL ".cc")
            list(APPEND aut_LIBS ${aut_FILE})
        endif("${aut_EXT}" STREQUAL ".cc")
    endforeach(aut_FILE)

    # Create a target to build the unit tests. Place the final executable in the build tree, not in the
    # top-level bin directory.
    #
    add_executable(${aut_NAME} ${aut_SRCS})
    set_target_properties(${aut_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(${aut_NAME} ${aut_LIBS} UnitTest)

    # !!! Yuck - specifying ${QT_LIBRARIES} here due to a problem with Apple compiles.
    #
    if(APPLE)
        target_link_libraries(${aut_NAME} ${QT_LIBRARIES})
    endif(APPLE)

    # Make the log output file and the sentinal file temporary.
    #
    set(aut_OK "${CMAKE_CURRENT_BINARY_DIR}/${aut_NAME}.ok")
    set(aut_LOG "${CMAKE_CURRENT_BINARY_DIR}/${aut_NAME}.log")
    set_source_files_properties("${aut_LOG}" "${aut_OK}" PROPERTIES GENERATED TRUE)

    # Execute the unit test and spit out the test results when the test fails. If the unit test succeeds, we
    # 'touch' a sentinal file that lets the CMake build know that everything worked.
    #
    add_custom_command(OUTPUT "${aut_OK}"
                       COMMAND ${CMAKE_COMMAND} -E remove ${aut_OK} ${aut_LOG}
                       COMMAND ${aut_NAME} > ${aut_LOG} 2>&1 || (cat ${aut_LOG} && false)
                       COMMAND ${CMAKE_COMMAND} -E touch "${aut_OK}"
                       BYPRODUCTS "${aut_LOG}"
                       DEPENDS ${aut_NAME}
                       COMMENT "Running unit tests in ${aut_NAME}" )

    # Create a custom target that will execute the above custom command whenever necessary to regenerate the
    # test output file
    #
    add_custom_target(${aut_NAME}-PASSED ALL DEPENDS ${aut_OK} ${aut_NAME})

endmacro(ADD_UNIT_TEST)

macro(add_tested_library NAME)

    if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    set(atl_TESTED ${NAME})
    set(atl_UNTESTED "${NAME}_tests")
    set(atl_OBJS "${NAME}_objs")
    set(atl_SRCS)
    set(atl_DEPS)
    set(atl_TESTS)

    set(atl_TAG atl_SRCS)
    foreach(atl_ARG ${ARGN})
        if("${atl_ARG}" STREQUAL "SOURCES")
            set(atl_TAG atl_SRCS)
        elseif("${atl_ARG}" STREQUAL "DEPS")
            set(atl_TAG atl_DEPS)
        elseif("${atl_ARG}" STREQUAL "TEST")
            set(atl_TAG atl_TESTS)
            list(APPEND ${atl_TAG} "TEST")
        else()
            list(APPEND ${atl_TAG} ${atl_ARG})
        endif()
    endforeach()

    # message("${atl_UNTESTED} sources: ${atl_SRCS}")
    # message("${atl_UNTESTED} deps: ${atl_DEPS}")

    add_library(${atl_OBJS} OBJECT ${atl_SRCS})
    add_library(${atl_UNTESTED} SHARED $<TARGET_OBJECTS:${atl_OBJS}>)
    target_link_libraries(${atl_UNTESTED} ${atl_DEPS})

    set(atl_TEST_TARGETS)
    set(atl_TST)

    foreach(atl_ARG ${atl_TESTS})
        if("${atl_ARG}" STREQUAL "TEST")
            if(atl_TST)
                add_unit_test(${atl_TST} ${atl_UNTESTED})
                list(APPEND atl_TEST_TARGETS ${aut_NAME}-PASSED)
            endif()
            set(atl_TST)
        else()
            list(APPEND atl_TST ${atl_ARG})
        endif()
    endforeach()

    if(atl_TST)
        add_unit_test(${atl_TST} ${atl_UNTESTED})
        list(APPEND atl_TEST_TARGETS ${aut_NAME}-PASSED)
    endif()

    # message("${atl_UNTESTED} tests: ${atl_TEST_TARGETS}")

    add_library(${atl_TESTED} SHARED $<TARGET_OBJECTS:${atl_OBJS}>)
    target_link_libraries(${atl_TESTED} ${atl_DEPS})
    set_target_properties(${atl_TESTED} PROPERTIES VERSION ${SIDECAR_VERSION} SOVERSION ${SIDECAR_VERSION})

    # add_dependencies(${atl_TESTED} ${atl_TEST_TARGETS})

endmacro(add_tested_library)
