# -*- Mode: CMake -*-
# 
# CMake build file for the 'playback' application.
# 

ADD_GUI( playback
		 main.cc
		 App.cc
		 App.h
		 BrowserWindow.cc
		 BrowserWindow.h
		 BrowserWindow.ui
		 Clock.cc
		 Clock.h
		 Emitter.cc
		 Emitter.h
		 FileModel.cc
		 FileModel.h
		 LoaderThread.cc
		 LoaderThread.h
		 NotesWindow.cc
		 NotesWindow.h
		 NotesWindow.ui
		 MainWindow.cc
		 MainWindow.h
		 MainWindow.ui
		 RecordingInfo.cc
		 RecordingInfo.h
		 RecordingModel.cc
		 RecordingModel.h
		 icons.qrc )
