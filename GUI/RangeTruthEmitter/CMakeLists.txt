# -*- Mode: CMake -*-
# 
# CMake build file for the 'rangetruthemitter' application.
# 

add_gui(rangetruthemitter
		main.cc
		AccelerationEditor.cc
		AccelerationEditor.h
		AddressLineEdit.cc
		AddressLineEdit.h
		AddressValidator.cc
		AddressValidator.h
		App.cc
		App.h
		DurationEditor.cc
		DurationEditor.h
		MainWindow.cc
		MainWindow.h
		MainWindow.ui
		SegmentModel.cc
		SegmentModel.h
		VelocityEditor.cc
		VelocityEditor.h)
