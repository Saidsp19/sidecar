# -*- Mode: CMake -*-
# 
# CMake build file for the 'siggen' application.
# 

add_gui(siggen
		main.cc
		App.cc
		App.h
		Emitter.cc
		Emitter.h
		MainWindow.cc
		MainWindow.h
		MainWindow.ui
		GeneratorConfiguration.cc
		GeneratorConfiguration.h
		GeneratorConfiguration.ui
		GeneratorConfigurationsModel.cc
		GeneratorConfigurationsModel.h)
