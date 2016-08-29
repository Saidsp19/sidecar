# -*- Mode: CMake -*-
# 
# CMake build file for the 'hands' application.
# 

add_gui(hands
		main.cc
		App.cc
		App.h
		ChannelConnection.cc
		ChannelConnection.h
		ChannelConnectionModel.cc
		ChannelConnectionModel.h
		ChannelConnectionView.cc
		ChannelConnectionView.h
		ChannelPlotConfiguration.ui
		ChannelPlotSettings.cc
		ChannelPlotSettings.h
		ChannelPlotWidget.cc
		ChannelPlotWidget.h
		ChannelPlotWidget.ui
		ConfigurationWindow.cc
		ConfigurationWindow.h
		ConfigurationWindow.ui
		MainWindow.cc
		MainWindow.h
		MainWindow.ui
		SamplePlotWidget.cc
		SamplePlotWidget.h)
