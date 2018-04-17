#include "QtCore/QSettings"
#include "QtWidgets/QStatusBar"
#include "QtWidgets/QVBoxLayout"

#include "GUI/LogUtils.h"
#include "GUI/PhantomCursorImaging.h"
#include "GUI/SvgIconMaker.h"
#include "GUI/ToolBar.h"
#include "GUI/Utils.h"
#include "Utils/Utils.h"

#include "App.h"
#include "Configuration.h"
#include "CursorWidget.h"
#include "MagnifierView.h"
#include "MagnifierWindow.h"
#include "PPIWidget.h"

#include "ui_MagnifierWindow.h"

using namespace SideCar::GUI::PPIDisplay;

int MagnifierWindow::id_ = 0;

Logger::Log&
MagnifierWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ppidisplay.MagnifierWindow");
    return log_;
}

MagnifierWindow::MagnifierWindow(PPIWidget* renderer)
    : Super(), gui_(new Ui::MagnifierWindow), renderer_(renderer)
{
    gui_->setupUi(this);

#ifdef Q_WS_X11
    setWindowFlags(windowFlags() | Qt::Tool | Qt::WindowStaysOnTopHint);
#else
    setWindowFlags(windowFlags() | Qt::Tool);
#endif

    setWindowTitle(QString("PPI Magnifier - %1").arg(++id_));
    setObjectName("MagnifierWindow");

    // Don't let the GUI::WindowManager save our state when quitting the application. Our master PPIWidget does
    // that instead.
    //
    setRestorable(false);

    QVBoxLayout* layout = new QVBoxLayout(gui_->centralwidget);
    layout->setMargin(0);
    layout->setSpacing(0);

    view_ = new MagnifierView(renderer, this);
    layout->addWidget(view_);
    renderer->addMagnifier(this);

    SvgIconMaker im;
    gui_->actionViewTogglePhantomCursor_->setIcon(im.make("phantomCursor"));
    gui_->actionViewToggleShowCursorPosition_->setIcon(im.make("cursorPosition"));

    CursorWidget* cursorWidget = new CursorWidget(statusBar());
    statusBar()->addPermanentWidget(cursorWidget);

    App* app = App::GetApp();
    connect(app, SIGNAL(phantomCursorChanged(const QPointF&)), cursorWidget, SLOT(showCursorPosition(const QPointF&)));

    ToolBar* toolBar = makeToolBar("Mag View", Qt::TopToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea | Qt::LeftToolBarArea | Qt::RightToolBarArea);
    toolBar->addAction(gui_->actionViewZoomIn_);
    toolBar->addAction(gui_->actionViewZoomOut_);
    toolBar->addAction(gui_->actionViewPanLeft_);
    toolBar->addAction(gui_->actionViewPanRight_);
    toolBar->addAction(gui_->actionViewPanDown_);
    toolBar->addAction(gui_->actionViewPanUp_);
    toolBar->addAction(gui_->actionViewToggleShowCursorPosition_);
    toolBar->addAction(gui_->actionViewTogglePhantomCursor_);

    connect(gui_->actionViewToggleToolbar_, SIGNAL(triggered()), toolBar->toggleViewAction(), SLOT(trigger()));

    connect(gui_->actionViewToggleOutline_, SIGNAL(triggered()), renderer, SLOT(update()));

    connect(gui_->actionViewToggleShowCursorPosition_, SIGNAL(toggled(bool)), SLOT(setShowCursorPosition(bool)));

    connect(gui_->actionViewTogglePhantomCursor_, SIGNAL(toggled(bool)), SLOT(setShowPhantomCursor(bool)));

    Configuration* configuration = app->getConfiguration();
    PhantomCursorImaging* phantomCursorImaging = configuration->getPhantomCursorImaging();
    connect(phantomCursorImaging, SIGNAL(enabledChanged(bool)), gui_->actionViewTogglePhantomCursor_,
            SLOT(setEnabled(bool)));
    setShowPhantomCursor(phantomCursorImaging->isEnabled());

    connect(gui_->actionViewZoomIn_, SIGNAL(triggered()), view_, SLOT(zoomIn()));
    connect(gui_->actionViewZoomOut_, SIGNAL(triggered()), view_, SLOT(zoomOut()));
    connect(gui_->actionViewPanLeft_, SIGNAL(triggered()), view_, SLOT(panLeft()));
    connect(gui_->actionViewPanRight_, SIGNAL(triggered()), view_, SLOT(panRight()));
    connect(gui_->actionViewPanDown_, SIGNAL(triggered()), view_, SLOT(panDown()));
    connect(gui_->actionViewPanUp_, SIGNAL(triggered()), view_, SLOT(panUp()));

    move(40, 40);
}

void
MagnifierWindow::setBounds(double xMin, double yMin, double width, double height)
{
    const int kMaxViewWidth = 700;
    const int kMaxViewHeight = 700;

    double scale;
    if (width > height) {
        // Calculate scale transform from pixels to real-world.
        //
        scale = width / double(kMaxViewWidth);

        // Use kMaxViewWidth for view widget width. Use scaled kMaxViewHeight for view widget height.
        //
        viewWidth_ = kMaxViewWidth;
        viewHeight_ = int(::rint(height / scale));

        // Recalculate real-world height to account for rounding of view widget height.
        //
        height = viewHeight_ * scale;
    } else {
        // Calculate scale transform from pixels to real-world.
        //
        scale = height / double(kMaxViewHeight);

        // Use kMaxViewHeight for view widget height. Use scaled kMaxViewWidth for view widget width.
        //
        viewHeight_ = kMaxViewHeight;
        viewWidth_ = int(::rint(width / scale));

        // Recalculate real-world width to account for rounding of view widget width.
        //
        width = viewWidth_ * scale;
    }

    view_->setBounds(xMin + width / 2.0, yMin + height / 2.0, scale);
}

void
MagnifierWindow::save(QSettings& settings) const
{
    view_->save(settings);
    settings.setValue("Geometry", saveGeometry());
    settings.setValue("ShowOutline", gui_->actionViewToggleOutline_->isChecked());
    settings.setValue("ShowCursorPosition", gui_->actionViewToggleShowCursorPosition_->isChecked());
    settings.setValue("ShowPhantomCursor", gui_->actionViewTogglePhantomCursor_->isChecked());
}

void
MagnifierWindow::restore(QSettings& settings)
{
    viewWidth_ = 0;
    viewHeight_ = 0;
    view_->restore(settings);
    restoreGeometry(settings.value("Geometry").toByteArray());
    setShowCursorPosition(settings.value("ShowCursorPosition", true).toBool());
    setShowPhantomCursor(settings.value("ShowPhantomCursor", true).toBool());
    gui_->actionViewToggleOutline_->setChecked(settings.value("ShowOutline", false).toBool());
}

void
MagnifierWindow::drawFrame() const
{
    view_->drawFrame();
}

void
MagnifierWindow::showEvent(QShowEvent* event)
{
    Super::showEvent(event);
    if (viewWidth_ && viewHeight_) {
        QSize delta(viewWidth_, viewHeight_);
        delta -= (view_->size());
        resize(size() + delta);
    }

    view_->setFocus();
}

void
MagnifierWindow::closeEvent(QCloseEvent* event)
{
    Super::closeEvent(event);

    // Tricky! If we are not quitting, then the user closed us, so inform our master PPIWidget that we no longer
    // exist. Otherwise, we stay connected to our master PPIWidget so that it may save our geometry
    // configuration before the application exits.
    //
    if (!getApp()->isQuitting()) renderer_->removeMagnifier(this);
}

void
MagnifierWindow::setShowPhantomCursor(bool state)
{
    GUI::UpdateToggleAction(gui_->actionViewTogglePhantomCursor_, state);
    view_->setShowPhantomCursor(state);
}

void
MagnifierWindow::setShowCursorPosition(bool state)
{
    GUI::UpdateToggleAction(gui_->actionViewToggleShowCursorPosition_, state);
    view_->setShowCursorPosition(state);
}

bool
MagnifierWindow::showOutline() const
{
    return gui_->actionViewToggleOutline_->isChecked();
}
