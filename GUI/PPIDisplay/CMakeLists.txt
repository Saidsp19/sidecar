# -*- Mode: CMake -*-
# 
# CMake build file for the 'ppidisplay' application.
# 
add_gui(ppidisplay
		main.cc
		App.cc
		App.h
		BackgroundImageSettings.cc
		BackgroundImageSettings.h
		BinaryVertexGenerator.cc
		Configuration.cc
		ConfigurationWindow.cc
		ConfigurationWindow.h
		ConfigurationWindow.ui
		ControlWidget.cc
		ControlWidget.h
		ControlsWindow.cc
		ControlsWindow.h
		ControlsWindow.ui
		CursorPosition.cc
		CursorPosition.h
		CursorWidget.cc
		CursorWidget.h
		DecaySettings.cc
		History.cc
		History.h
		HistorySettings.cc
		HistorySettings.h
		HistoryWidget.cc
		HistoryWidget.h
		MagnifierView.cc
		MagnifierView.h
		MagnifierWindow.cc
		MagnifierWindow.h
		MagnifierWindow.ui
		MainWindow.cc
		MainWindow.h
		MainWindow.ui
		OffscreenBuffer.cc
		OffscreenBuffer.h
		PlotPositionFunctor.cc
		PPIWidget.cc
		PPIWidget.h
		VideoImaging.cc
		VideoImaging.h
		VideoVertexGenerator.cc
		ViewSettings.cc
		ViewSettings.h
        ppidisplay.icns
        ppidisplay.png
        )

set_target_properties(ppidisplay PROPERTIES MACOSX_BUNDLE_INFO_STRING
                      "Polar display of SideCar signals and plots.")
