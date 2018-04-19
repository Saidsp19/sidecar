# -*- Mode: CMake -*-
# 
# CMake build file for the 'priemitter' application.
# 

add_gui(priemitter
		main.cc
		App.cc
		App.h
		MainWindow.cc
		MainWindow.h
		MainWindow.ui
        priemitter.icns
        priemitter.png
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        )

set_target_properties(priemitter PROPERTIES MACOSX_BUNDLE_INFO_STRING
                      "Basic PRI emitter for one channel.")
