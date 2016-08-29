# -*- Mode: CMake -*-
# 
# CMake build file for the 'spectrum' application.
# 

include_directories(${FFTW3_INCLUDE_DIRS})

add_gui(spectrum
		main.cc
		App.cc
		App.h
		AzimuthLatch.cc
		AzimuthLatch.h
		AzimuthLatch.ui
		ChannelSelectorWidget.cc
		ChannelSelectorWidget.h
		ChannelSelectorWidget.ui
		ColorMapWidget.cc
		ColorMapWidget.h
		ColorMapWidget.ui
		Configuration.cc
		Configuration.h
		ConfigurationWindow.cc
		ConfigurationWindow.h
		ConfigurationWindow.ui
		CursorWidget.cc
		CursorWidget.h
		FFTSettings.cc
		FFTSettings.h
		FreqScaleWidget.cc
		MainWindow.cc
		MainWindow.h
		MainWindow.ui
		PowerScaleWidget.cc
		ScaleWidget.cc
		ScaleWidget.h
		Settings.cc
		Settings.h
		SpectrographImaging.cc
		SpectrographImaging.h
		SpectrographScrollArea.cc
		SpectrographScrollArea.h
		SpectrographWidget.cc
		SpectrographWidget.h
		SpectrographWindow.cc
		SpectrographWindow.h
		SpectrographWindow.ui
		SpectrumWidget.cc
		SpectrumWidget.h
		ViewChanger.cc
		ViewChooser.cc
		ViewChooser.h
		ViewEditor.cc
		ViewEditor.h
		ViewEditor.ui
		ViewSettings.cc
		WeightWindow.cc
		WeightWindow.h
		WorkRequest.cc
		WorkRequest.h
		WorkerThread.cc
		WorkerThread.h
		icons.qrc
        spectrum.icns
        spectrum.png
        )

set_target_properties(spectrum PROPERTIES MACOSX_BUNDLE_INFO_STRING "Spectrum viewer for SideCar signals.")

target_link_libraries(spectrum ${FFTW3_LIBRARIES})
