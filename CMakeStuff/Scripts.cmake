# -*- Mode: CMake -*-
#
# Macro ADD_SCRIPTS [ FILE ... ]
#
macro(ADD_SCRIPTS)

    # Create destination build directory if necessary
    #
    if(NOT EXISTS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
        file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    endif(NOT EXISTS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

    # Iterate over all of the script names, creating a production rule for each
    #
    foreach(as_TARGET ${ARGV})

        # Define full paths for source and destination files
        #
        get_filename_component(as_EXT "${as_TARGET}" EXT)
        set(as_SRC  "${CMAKE_CURRENT_SOURCE_DIR}/${as_TARGET}")

	    # Get target name w/out extention (NAME_WE)
	    #
        get_filename_component(as_TARGET_WE "${as_TARGET}" NAME_WE)
        set(as_DST "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${as_TARGET_WE}")

	    # What we do is dependent on the file type.
	    #
        if("${as_EXT}" MATCHES "\\.[k]?sh")

	        # Bash or Bourne shell files
	        #
            add_custom_command(OUTPUT "${as_DST}"
                               COMMAND $ENV{SHELL} -n "${as_SRC}"
     	                       COMMAND ${CMAKE_COMMAND} -E copy "${as_SRC}" "${as_DST}"
		                       COMMAND chmod a+x,go-w "${as_DST}"
      	                       DEPENDS ${as_SRC})

        else("${as_EXT}" MATCHES "\\.[k]?sh")

	        # Anything else, we just copy over, keeping the extension
	        #
            add_custom_command(OUTPUT "${as_DST}"
     	                       COMMAND ${CMAKE_COMMAND} -E copy "${as_SRC}" "${as_DST}"
		                       COMMAND chmod a+x,go-w "${as_DST}"
      	                       DEPENDS ${as_SRC})

        endif("${as_EXT}" MATCHES "\\.[k]?sh")

        add_custom_target(Script_${as_TARGET_WE} ALL DEPENDS ${as_DST})

	    # Make sure that CMake will remake it if necessary
	    #
        set_source_files_properties("${as_DST}" PROPERTIES GENERATED TRUE)
	    install(PROGRAMS ${as_DST} DESTINATION bin)

    endforeach(as_TARGET)

endmacro(ADD_SCRIPTS)

