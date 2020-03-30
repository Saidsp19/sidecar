#include "QtCore/QSettings"
#include "QtCore/QTimer"
#include "QtGui/QCloseEvent"
#include "QtGui/QPainter"
#include "QtWidgets/QGridLayout"
#include "QtWidgets/QStatusBar"
#include "QtWidgets/QToolButton"

#include "GUI/ChannelInfoWidget.h"
#include "GUI/ChannelMenu.h"
#include "GUI/LogUtils.h"
#include "GUI/RadarInfoWidget.h"
#include "GUI/ToolBar.h"
#include "GUI/Utils.h"
#include "GUI/WindowManager.h"

#include "AzimuthLatch.h"
#include "ChannelSelectorWidget.h"
#include "Configuration.h"
#include "CursorWidget.h"
#include "FFTSettings.h"
#include "FreqScaleWidget.h"
#include "MainWindow.h"
#include "PowerScaleWidget.h"
#include "Settings.h"
#include "SpectrographWindow.h"
#include "SpectrumWidget.h"
#include "ViewChooser.h"
#include "ViewEditor.h"
#include "ViewSettings.h"

using namespace SideCar::GUI::Spectrum;
using namespace SideCar::Messages;

Logger::Log&
MainWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("spectrum.MainWindow");
    return log_;
}

MainWindow::MainWindow() :
    MainWindowBase(), Ui::MainWindow(), display_(nullptr), powerScale_(nullptr), freqScale_(nullptr)
{
    static Logger::ProcLog log("MainWindow", Log());
    LOGINFO << std::endl;

    setupUi(this);
    setObjectName("MainWindow");
#ifdef __DEBUG__
    setWindowTitle("Spectrum Analyzer (DEBUG)");
#else
    setWindowTitle("Spectrum Analyzer");
#endif

    QWidget* top = centralWidget();
    QGridLayout* layout = new QGridLayout;
    top->setLayout(layout);

    display_ = new SpectrumWidget(top);

    // NOTE: connected asynchronously so that the background is redrawn after the scale widgets have resized.
    //
    connect(display_, &SpectrumWidget::transformChanged, this, &MainWindow::transformChanged,
            Qt::QueuedConnection);
    connect(display_, &SpectrumWidget::currentCursorPosition, this, &MainWindow::showCursorPosition);

    freqScale_ = new FreqScaleWidget(top, Qt::Horizontal);
    freqScale_->setLabel("Frequency");
    freqScale_->setAutoDivide(true);
    freqScale_->setMajorTickDivisions(5);

    powerScale_ = new PowerScaleWidget(top, Qt::Vertical);
    powerScale_->setLabel("dBFS");
    powerScale_->setAutoDivide(true);
    powerScale_->setMajorTickDivisions(5);

    layout->setSpacing(0);
    layout->addWidget(powerScale_, 0, 0);
    layout->addWidget(display_, 0, 1);
    layout->addWidget(freqScale_, 1, 1);

    App* app = getApp();
    Configuration* cfg = app->getConfiguration();
    Settings* settings = cfg->getSettings();

    actionViewPrevious_->setIcon(QIcon(":/popViewStack.png"));
    actionViewPrevious_->setEnabled(false);
    actionViewFull_->setIcon(QIcon(":/home.png"));
    actionViewSwap_->setIcon(QIcon(":/refresh.png"));
    actionViewSwap_->setEnabled(false);

    // Cursor info widget in the status bar
    //
    cursorWidget_ = new CursorWidget(statusBar());
    statusBar()->addPermanentWidget(cursorWidget_);

    // Radar info widget in the status bar
    //
    RadarInfoWidget* radarInfoWidget = new RadarInfoWidget(statusBar());
    connect(display_, &SpectrumWidget::showMessageInfo, radarInfoWidget, &RadarInfoWidget::showMessageInfo);
    statusBar()->addPermanentWidget(radarInfoWidget);

    // Connection info widget in the status bar
    //
    ChannelInfoWidget* channelInfoWidget = new ChannelInfoWidget(statusBar());
    statusBar()->addPermanentWidget(channelInfoWidget);
    channelInfoWidget->addChannel("In: ", settings->getInputChannel());

    // Tool windows toolbar
    //
    ToolBar* toolBar = makeToolBar("Channel", Qt::TopToolBarArea);
    ChannelSelectorWidget* csw = new ChannelSelectorWidget(toolBar);
    toolBar->addWidget(csw);

    ChannelSetting* channelSetting = settings->getInputChannel();
    channelSetting->connectWidget(csw->found_);

    connect(channelSetting, &StringSetting::valueChanged, this, &MainWindow::channelChanged);
    if (channelSetting->isConnected()) channelChanged(channelSetting->getValue());

    // View toolbar
    //
    toolBar = makeToolBar("View", Qt::TopToolBarArea);

    connect(settings, &Settings::showGridChanged, this, &MainWindow::makeBackground);
    connect(settings, &Settings::showGridChanged, actionViewToggleGrid_, &QAction::setChecked);

    connect(actionViewToggleGrid_, &QAction::toggled, app, &App::updateToggleAction);
    connect(actionViewToggleGrid_, &QAction::triggered, settings, &Settings::setShowGrid);

    actionViewToggleGrid_->setIcon(QIcon(":/gridOn.png"));
    UpdateToggleAction(actionViewToggleGrid_, settings->getShowGrid());

    QAction* action = app->getToolsMenuAction(App::kShowConfigurationWindow);
    action->setIcon(QIcon(":/configurationWindow.png"));
    toolBar->addAction(action);

    toolBar->addAction(actionViewToggleGrid_);

    actionViewPause_->setData(QStringList() << "Freeze" << "Unfreeze");
    UpdateToggleAction(actionViewPause_, false);
    actionViewPause_->setIcon(QIcon(":/unfreeze.png"));
    connect(actionViewPause_, &QAction::triggered, app, &App::updateToggleAction);
    toolBar->addAction(actionViewPause_);

    toolBar->addAction(actionViewFull_);
    toolBar->addAction(actionViewPrevious_);
    toolBar->addAction(actionViewSwap_);
    app->addWindowMenuActionTo(toolBar, WindowManager::kFullScreen);

    // FFT config toolbar
    //
    FFTSettings* fftSettings = cfg->getFFTSettings();
    toolBar = makeToolBar("FFT Config", Qt::TopToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);

    auto label = new QLabel("FFT Size:", toolBar);
    label->setForegroundRole(QPalette::WindowText);
    toolBar->addWidget(label);
    auto w = fftSettings->duplicateFFTSizePower(toolBar);
    w->setFocusPolicy(Qt::NoFocus);
    toolBar->addWidget(w);

    label = new QLabel("Wndow:", toolBar);
    label->setForegroundRole(QPalette::WindowText);
    toolBar->addWidget(label);
    w = fftSettings->duplicateWindowType(toolBar);
    w->setFocusPolicy(Qt::NoFocus);
    toolBar->addWidget(w);

    // Azimuth Trigger config toolbar
    //
    toolBar = makeToolBar("Azimuth Latch", Qt::TopToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    AzimuthLatch* azimuthLatch = new AzimuthLatch(toolBar);
    toolBar->addWidget(azimuthLatch);
    display_->setAzimuthLatch(azimuthLatch);

    // View Editor/Chooser toolbar
    //
    toolBar = makeToolBar("View Chooser", Qt::TopToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    action = app->getToolsMenuAction(App::kShowViewEditor);
    action->setIcon(QIcon(":/viewEditor.svg"));
    toolBar->addAction(action);
    ViewChooser* chooser = new ViewChooser(app->getViewEditor(), toolBar);
    chooser->setFocusPolicy(Qt::NoFocus);
    toolBar->addWidget(chooser);

    transformChanged();
}

void
MainWindow::closeEvent(QCloseEvent* event)
{
    App* app = getApp();

    if (!app->isQuitting()) {
        event->ignore();
        QTimer::singleShot(0, app, SLOT(applicationQuit()));
        return;
    }

    Super::closeEvent(event);
}

void
MainWindow::transformChanged()
{
    static Logger::ProcLog log("transformChanged", Log());
    LOGINFO << std::endl;
    updateScales();
    makeBackground();
    actionViewPrevious_->setEnabled(display_->canPopView());
    actionViewSwap_->setEnabled(display_->canPopView());
    actionViewFull_->setEnabled(display_->canPopView());
}

void
MainWindow::makeBackground()
{
    QImage image(display_->width(), display_->height(), QImage::Format_RGB32);
    if (!image.isNull()) {
        QPainter painter(&image);
        painter.setBackground(Qt::black);
        painter.eraseRect(image.rect());
        if (getApp()->getConfiguration()->getSettings()->getShowGrid()) {
            powerScale_->drawGridLines(painter);
            freqScale_->drawGridLines(painter);
        }
        painter.end();
        display_->setBackground(image);
    }
}

void
MainWindow::updateScales()
{
    static Logger::ProcLog log("updateScales", Log());
    LOGINFO << std::endl;
    const ViewSettings& settings(display_->getCurrentView());
    LOGINFO << "xMin: " << settings.getXMin() << " xMax: " << settings.getXMax() << " yMin: " << settings.getYMin()
            << " yMax: " << settings.getYMax() << std::endl;
    freqScale_->setStart(settings.getXMin());
    freqScale_->setEnd(settings.getXMax());
    powerScale_->setStart(settings.getYMax());
    powerScale_->setEnd(settings.getYMin());
}

void
MainWindow::showCursorPosition(const QPointF& pos)
{
    freqScale_->setCursorValue(pos.x());
    powerScale_->setCursorValue(pos.y());
    cursorWidget_->showCursorPosition(
        QString("X: %1 Y: %2 db").arg(freqScale_->formatTickTag(pos.x())).arg(powerScale_->formatTickTag(pos.y())));
}

void
MainWindow::on_actionViewPause__triggered(bool state)
{
    display_->setFrozen(state);
}

void
MainWindow::on_actionViewFull__triggered()
{
    display_->popAllViews();
    actionViewPrevious_->setEnabled(false);
    actionViewSwap_->setEnabled(false);
    actionViewFull_->setEnabled(false);
}

void
MainWindow::on_actionViewPrevious__triggered()
{
    display_->popView();
    actionViewPrevious_->setEnabled(display_->canPopView());
    actionViewSwap_->setEnabled(display_->canPopView());
    actionViewFull_->setEnabled(display_->canPopView());
}

void
MainWindow::on_actionViewSwap__triggered()
{
    display_->swapViews();
}

void
MainWindow::on_actionShowSpectrograph__triggered()
{
    App::GetApp()->getSpectrographWindow()->showAndRaise();
}

void
MainWindow::channelChanged(const QString& name)
{
    setWindowTitle(QString("Spectrum Analyser: %1").arg(name));
}

void
MainWindow::saveToSettings(QSettings& settings)
{
    Super::saveToSettings(settings);
    settings.beginGroup("SpectrumWidget");
    display_->saveToSettings(settings);
    settings.endGroup();
}

void
MainWindow::restoreFromSettings(QSettings& settings)
{
    Super::restoreFromSettings(settings);
    settings.beginGroup("SpectrumWidget");
    display_->restoreFromSettings(settings);
    settings.endGroup();
}
