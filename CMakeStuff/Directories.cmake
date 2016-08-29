# -*- Mode: CMake -*-
#
# Macro add_directories [ DIR1 ... ]
#
# Add a one or more directories to the build
#
macro(ADD_DIRECTORIES)
    foreach(TARGET ${ARGV})
        add_subdirectory(${TARGET})
    endforeach(TARGET)
endmacro(ADD_DIRECTORIES)
