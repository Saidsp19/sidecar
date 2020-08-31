# -*- Mode: CMake -*-
#
# Macro add_algorithm NAME
#
# Create appropriate build rules for an algorithm. NAME is the name of the algorithm to make and is the name of
# the build target.
#
macro(ADD_ALGORITHM NAME)

    # Create the output directory if necessary
    #
    if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}")

    # Fetch the set of files to use to build the algorithm
    #
    set(FILES ${ARGN})

    # See if there is a file call ${NAME}.axml in the source directory. If so, use it to generate a
    # ${NAME}_defauls.h file. Uses the algxml Python script to do the generation.
    #
    set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.axml")
    if(EXISTS "${SRC}")
        set(DST "${CMAKE_CURRENT_BINARY_DIR}/${NAME}_defaults.h")
        add_custom_command(OUTPUT "${DST}"
                           COMMAND python ${PROJECT_BINARY_DIR}/bin/algxml -o ${DST} ${SRC}
                           DEPENDS "${SRC}" ${PROJECT_BINARY_DIR}/bin/algxml)
        set_source_files_properties("${DST}" PROPERTIES GENERATED TRUE)
        list(APPEND FILES ${DST})
    endif(EXISTS "${SRC}")

    # Make sure that the algorithm can see the ${NAME}_defaults.h file in the binary directory.
    #
    include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

    # Build the algorithm DLL. Link with the Algorithm library.
    #
    add_library(${NAME} SHARED ${FILES})
    set_target_properties(${NAME} PROPERTIES VERSION ${SIDECAR_VERSION} SOVERSION ${SIDECAR_VERSION})
    target_link_libraries(${NAME} Algorithm ${QT_LIBRARIES} ${MATHLIBS})
    install(TARGETS ${NAME} LIBRARY DESTINATION lib)

endmacro(ADD_ALGORITHM)
