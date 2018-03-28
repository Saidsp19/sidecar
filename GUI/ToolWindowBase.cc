#include <algorithm> // for std::swap

#include "QtCore/QSettings"
#include "QtCore/QTimer"
#include "QtGui/QAction"
#include "QtGui/QApplication"
#include "QtGui/QDesktopWidget"
#include "QtGui/QFileDialog"
#include "QtGui/QHideEvent"
#include "QtGui/QLayout"
#include "QtGui/QMoveEvent"
#include "QtGui/QResizeEvent"
#include "QtGui/QShowEvent"
#include "QtGui/QStatusBar"

#include "AppBase.h"
#include "LogUtils.h"
#include "MainWindowBase.h"
#include "ToolWindowBase.h"
#include "Utils.h"

using namespace SideCar::GUI;

static const char* const kInitialPosition = "InitialPosition";
static const char* const kInitialSize = "InitialSize";
static const char* const kInitialVisibility = "InitialVisibility";

struct Settings : QSettings {
    Settings(const QString& name) : QSettings() { beginGroup(name); }
    ~Settings() { endGroup(); }
};

Logger::Log&
ToolWindowBase::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.ToolWindowBase");
    return log_;
}

ToolWindowBase::ToolWindowBase(const QString& name, const QString& actionTitle, int shortcut) :
    QDialog(AppBase::GetApp() ? AppBase::GetApp()->getActiveMainWindow() : 0),
    showHideAction_(MakeMenuAction(QString("%1 Window").arg(actionTitle), this, SLOT(toggleVisibility()), shortcut)),
    restoreFromSettings_(true)
{
    Logger::ProcLog log("ToolWindowBase", Log());
    LOGINFO << name << std::endl;
    setObjectName(name);

    // At least for Gnome (version ?) the Qt::Tool does little but cause frustration. However, still keep the
    // window on top of the main one.
    //
#ifdef Q_WS_X11
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
#else
    setWindowFlags(windowFlags() | Qt::Tool);
#endif

    if (getApp()) {
        connect(getApp(), SIGNAL(activeMainWindowChanged(MainWindowBase*)),
                SLOT(activeMainWindowChanged(MainWindowBase*)));
    }

    showHideAction_->setCheckable(true);
    updateShowHideMenuAction(false);
}

AppBase*
ToolWindowBase::getApp()
{
    return AppBase::GetApp();
}

void
ToolWindowBase::setFixedSize()
{
    static Logger::ProcLog log("setFixedSize", Log());
    LOGINFO << objectName() << std::endl;
    setSizeGripEnabled(false);
    if (layout()) layout()->setSizeConstraint(QLayout::SetFixedSize);
}

void
ToolWindowBase::showEvent(QShowEvent* event)
{
    static Logger::ProcLog log("showEvent", Log());
    LOGINFO << objectName() << std::endl;
    if (restoreFromSettings_) {
        restoreFromSettings_ = false;
        Settings settings(objectName());
        restoreGeometry(settings.value("Geometry").toByteArray());
    }

    updateShowHideMenuAction(true);
    Super::showEvent(event);
}

void
ToolWindowBase::closeEvent(QCloseEvent* event)
{
    static Logger::ProcLog log("closeEvent", Log());
    LOGINFO << std::endl;

    Settings settings(objectName());
    settings.setValue("Geometry", saveGeometry());

    // If the closeEvent is due to the application quitting, then remember that we were open before quitting.
    // Otherwise, the window was closed due to a user a action.
    //
    settings.setValue("Visible", getApp()->isQuitting());
    updateShowHideMenuAction(false);
    Super::closeEvent(event);
}

void
ToolWindowBase::hideEvent(QHideEvent* event)
{
    static Logger::ProcLog log("hideEvent", Log());
    LOGINFO << std::endl;

    // Don't mess with these settings if the application is quitting; they would be handled from within the
    // closeEvent() method. We only want to update the values when the user manually closes the window.
    //
    if (!getApp()->isQuitting()) {
        Settings settings(objectName());
        settings.setValue("Geometry", saveGeometry());
        settings.setValue("Visible", false);
        updateShowHideMenuAction(false);
    }

    Super::hideEvent(event);
}

bool
ToolWindowBase::getInitialVisibility() const
{
    static Logger::ProcLog log("getInitialVisibility", Log());
    Settings settings(objectName());
    bool value = settings.value("Visible", false).toBool();
    LOGINFO << objectName() << ' ' << value << std::endl;
    return value;
}

void
ToolWindowBase::applyInitialVisibility()
{
    if (getInitialVisibility()) showAndRaise();
}

void
ToolWindowBase::showAndRaise()
{
    show();
    raise();
    activateWindow();
}

void
ToolWindowBase::toggleVisibility()
{
    if (isVisible()) {
        close();
    } else {
        showAndRaise();
    }
}

void
ToolWindowBase::updateShowHideMenuAction(bool state)
{
    UpdateToggleAction(showHideAction_, state);
}

void
ToolWindowBase::activeMainWindowChanged(MainWindowBase* window)
{
#if 0
  if (window != parentWidget()) {
    bool visible = isVisible();
    setParent(window, windowFlags());
    if (visible)
      show();
  }
#endif
}
