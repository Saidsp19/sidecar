# -*- Mode: CMake -*-
# 
# CMake build file for the 'zcm' application.
# 

add_gui(zcm
		main.cc
		App.cc
		App.h
		BrowserWidget.cc
		BrowserWidget.h
		MainWindow.cc
		MainWindow.h
		MainWindow.ui
        zcm.icns
        zcm.png
        )

set_target_properties(zcm PROPERTIES MACOSX_BUNDLE_INFO_STRING
                      "Monitor SideCar Zerconf entities.")
