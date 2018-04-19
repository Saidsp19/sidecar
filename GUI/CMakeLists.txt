# -*- Mode: CMake -*-
# 
# CMake build file for the GUIUtils library and GUI applications.
# 

# Make sure that when we compile we see any include files in the current build directory.
#

set(QTDEPS Core Gui Network OpenGL Svg Widgets Xml)
set(QTLIBS)

foreach(tmp ${QTDEPS})
    add_definitions(${Qt5${tmp}_DEFINITIONS})
    include_directories(${Qt5${tmp}_INCLUDE_DIRS})
    list(APPEND QTLIBS Qt5::${tmp})
endforeach()

include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

qt5_add_resources(GUI_tQRCS LED.qrc guiIcons.qrc)

qt5_wrap_ui(GUI_tUIS
            ChannelInfoWidget.ui
            ChannelSelectorWindow.ui
            ColorMapWidget.ui
            ControlsWindow.ui
            CursorWidget.ui
            LoggerWindow.ui
            ManualWindow.ui
            PresetsWindow.ui
            RadarInfoWidget.ui
            TargetPlotSymbolsWidget.ui)

qt5_wrap_cpp(GUI_tMOCS AppBase.h
             BoolSetting.h
             BugPlotEmitterSettings.h
             ChannelGroup.h
             ChannelImaging.h
             ChannelInfoWidget.h
             ChannelMenu.h
             ChannelSelectorWindow.h
             ChannelSetting.h
             CLUTSetting.h
             ColorButtonSetting.h
             ColorButtonWidget.h
             ColorMapWidget.h
             ControlsWidget.h
             ControlsWindow.h
             CursorWidget.h
             DoubleMinMaxValidator.h
             DoubleSetting.h
             IntMinMaxValidator.h
             IntSetting.h
             LED.h
             LoggerModel.h
             LoggerTreeItem.h
             LoggerView.h
             LoggerWindow.h
             MainWindowBase.h
             ManualWindow.h
             MessageReader.h
             # modeltest.h
             MulticastMessageReader.h
             OffscreenBuffer.h
             OnOffSettingsBlock.h
             OpacitySetting.h
             PathSetting.h
             PlotSymbolWidget.h
             Potentiometer.h
             Preset.h
             PresetChooser.h
             PresetManager.h
             PresetsWindow.h
             PriorityWidget.h
             QColorSetting.h
             QComboBoxSetting.h
             QDoubleSpinBoxSetting.h
             QLineEditSetting.h
             QSliderSetting.h
             QSpinBoxSetting.h
             QTimeEditSetting.h
             QtMonitor.h
             RadarInfoWidget.h
             RangeMap.h
             RangeRingsImaging.h
             RangeTruthsImaging.h
             ReaderThread.h
             #              RTCLMessageReader.h <+ craps out during Qt MOC processing. Why?
             SampleImaging.h
             SCStyle.h
             ServiceBrowser.h
             ServiceEntry.h
             Setting.h
             SettingsBlock.h
             Splittable.h
             SquareButton.h
             StringSetting.h
             StringListSetting.h
             Subscriber.h
             SvgIconMaker.h
             TargetPlotImaging.h
             TargetPlotSymbolsWidget.h
             TCPMessageReader.h
             TimeSetting.h
             ToolBar.h
             ToolWindowBase.h
             VideoSampleCountTransform.h
             VTick.h
             WindowManager.h
             Writers.h
             )

# Production specification for libGUIUtils
#
add_library(GUIUtils SHARED
            AppBase.cc
            BoolSetting.cc
            BugPlotEmitterSettings.cc
            ChannelGroup.cc
            ChannelImaging.cc
            ChannelInfoWidget.cc
            ChannelMenu.cc
            ChannelSelectorWindow.cc
            ChannelSetting.cc
            CLUT.cc
            CLUTSetting.cc
            ColorButtonSetting.cc
            ColorButtonWidget.cc
            ColorMapWidget.cc
            ControlsWidget.cc
            ControlsWindow.cc
            CursorWidget.cc
            DoubleMinMaxValidator.cc
            DoubleSetting.cc
            GLFont.cc
            GLInitLock.cc
            IntMinMaxValidator.cc
            IntSetting.cc
            KeySeq.cc
            KeySetting.cc
            LED.cc
            LoggerModel.cc
            LoggerTreeItem.cc
            LoggerView.cc
            LoggerWindow.cc
            LogUtils.cc
            MainWindowBase.cc
            ManualWindow.cc
            MessageReader.cc
            # modeltest.cpp
            MulticastMessageReader.cc
            OffscreenBuffer.cc
            OnOffSettingsBlock.cc
            OpacitySetting.cc
            PathSetting.cc
            PhantomCursorImaging.cc
            PlotSymbolWidget.cc
            Potentiometer.cc
            Preset.cc
            PresetChooser.cc
            PresetManager.cc
            PresetsWindow.cc
            PriorityWidget.cc
            QCheckBoxSetting.cc
            QColorSetting.cc
            QComboBoxSetting.cc
            QDoubleSpinBoxSetting.cc
            QGroupBoxSetting.cc
            QLineEditSetting.cc
            QRadioButtonSetting.cc
            QSliderSetting.cc
            QSpinBoxSetting.cc
            QTabWidgetSetting.cc
            QTimeEditSetting.cc
            QtMonitor.cc
            RadarInfoWidget.cc
            RangeMap.cc
            RangeRingsImaging.cc
            RangeTruthsImaging.cc
            ReaderThread.cc
            #             RTCLMessageReader.cc
            SampleImaging.cc
            SCStyle.cc
            ServiceBrowser.cc
            ServiceEntry.cc
            Setting.cc
            SettingsBlock.cc
            Splittable.cc
            SquareButton.cc
            StencilBufferState.cc
            StringSetting.cc
            StringListSetting.cc
            Subscriber.cc
            SvgIconMaker.cc
            TargetPlot.cc
            TargetPlotImaging.cc
            TargetPlotSymbolsWidget.cc
            TCPMessageReader.cc
            Texture.cc
            TimeSetting.cc
            ToolBar.cc
            ToolWindowBase.cc
            Utils.cc
            VertexColorArray.cc
            VertexGenerator.cc
            VideoSampleCountTransform.cc
            VTick.cc
            WindowManager.cc
            Writers.cc

            ${GUI_tQRCS}
            ${GUI_tUIS}
            ${GUI_tMOCS}
            )

set_target_properties(GUIUtils PROPERTIES VERSION ${SIDECAR_VERSION} SOVERSION ${SIDECAR_VERSION})

# Linkage dependencies for libGUIUtils
#
message(STATUS "GUI linking with these OpenGL libraries: ${OPENGL_LIBRARIES}")
target_link_libraries(GUIUtils IO ${QTLIBS} ${OPENGL_LIBRARIES})
install(TARGETS GUIUtils LIBRARY DESTINATION lib)

# Unit test for CLUT class
#
add_unit_test(CLUTTests.cc GUIUtils)

# The Spectrum app compiles on macOS 10.13 but not on Fedora. Needs to be ported to current Qt5 OpenGL classes.
#
if(APPLE)
    set(SPECTRUM "Spectrum")
else(APPLE)
    set(SPECTRUM)
endif(APPLE)

add_directories(Alert
                AScope
                BScope
                ConfigEditor
                ESScope
                ExtractionEmitter
                HealthAndStatus
                Master
                Playback
                PPIDisplay
                PRIEmitter
                RangeTruthEmitter
                SignalGenerator
                ${SPECTRUM}
                TestBed
                ZeroconfMonitor
                )
