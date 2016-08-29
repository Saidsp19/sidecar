# -*- Mode: CMake -*-
#
# Macro add_gui [ TARGET ... ]
#
# Creates a target to generate a GUI application. The first parameter to the macro is the name of the
# application, followed by the source files to use to generate it. Currently, there are five types of files that
# can be specified:
#
# - C++ source files
# - C++ include files (*.h) for Qt 'moc' processing
# - Qt UI files (*.ui)
# - Qt resource files (*.qrc)
# - ICNS file for setting the Mac bundle's icons
# - PNG file for use in documentation and app switchers
#

macro(ADD_GUI NAME)

    set(${NAME}_QRCS)
    set(${NAME}_UIS)
    set(${NAME}_MOCS)
    set(${NAME}_CPPS)
    set(${NAME}_ICNS)
    set(${NAME}_PNG)
    
    foreach(FILE ${ARGN})
        get_filename_component(EXT "${FILE}" EXT)
        if(EXT STREQUAL ".ui")
            list(APPEND ${NAME}_UIS ${FILE})
        elseif(EXT STREQUAL ".h")
            list(APPEND ${NAME}_MOCS ${FILE})
        elseif(EXT STREQUAL ".qrc")
            list(APPEND ${NAME}_QRCS ${FILE})
        elseif(EXT STREQUAL ".icns")
            if(APPLE)
                set(${NAME}_ICNS ${FILE})
            endif(APPLE)
        elseif(EXT STREQUAL ".png")
            set(${NAME}_PNG ${FILE})
        else(EXT STREQUAL ".ui")
            list(APPEND ${NAME}_tCPPS ${FILE})
        endif(EXT STREQUAL ".ui")
    endforeach(FILE ${ARGN})

    include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

    qt4_add_resources(${NAME}_tQRCS ${${NAME}_QRCS})
    qt4_wrap_cpp(${NAME}_tMOCS ${${NAME}_MOCS})
    qt4_wrap_ui(${NAME}_tUIS ${${NAME}_UIS})

    add_executable(${NAME} ${SIDECAR_PACKAGING} ${${NAME}_tQRCS} ${${NAME}_tUIS} ${${NAME}_tMOCS}
                   ${${NAME}_tCPPS} ${${NAME}_ICNS} ${${NAME}_PNG})
    target_link_libraries(${NAME} ${OPENGL_LIBRARIES} ${QT_LIBRARIES} GUIUtils)

    if(APPLE)
        set_target_properties(${NAME} PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "com.bray.${NAME}")
        set_target_properties(${NAME} PROPERTIES MACOSX_BUNDLE_LONG_VERSION_STRING "v1.0.0")
        set_target_properties(${NAME} PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0")
        set_target_properties(${NAME} PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION "1.0")
        set_target_properties(${NAME} PROPERTIES MACOSX_BUNDLE_COPYRIGHT
                              "Copyright © 2016 Brad Howes. All rights reserved.")

        if(${NAME}_PNG)
            set_source_files_properties(${${NAME}_PNG} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        endif(${NAME}_PNG)

        if(${NAME}_ICNS)
            set_source_files_properties(${${NAME}_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
            set_target_properties(${NAME} PROPERTIES MACOSX_BUNDLE_ICON_FILE ${${NAME}_ICNS})
        endif(${NAME}_ICNS)

        install(TARGETS ${NAME} BUNDLE DESTINATION bin)

        # Let the app disable any OS sleeping
        #
        add_custom_command(TARGET ${NAME} POST_BUILD
                           COMMAND /usr/bin/defaults write ${MACOSX_BUNDLE_GUI_IDENTIFIER} NSAppSleepDisabled
                           -bool YES)
    else(APPLE)
        install(TARGETS ${NAME} RUNTIME DESTINATION bin)
    endif(APPLE)

endmacro(ADD_GUI NAME)
