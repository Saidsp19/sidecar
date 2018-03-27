#include <signal.h>

#include "QtCore/QFile"
#include "QtCore/QSettings"
#include "QtCore/QTimer"
#include "QtGui/QFileDialog"
#include "QtGui/QIcon"
#include "QtGui/QMenu"
#include "QtGui/QMenuBar"
#include "QtGui/QPixmap"
#include "QtGui/QSplashScreen"
#include "QtGui/QStatusBar"
#include "QtGui/QWidget"
#include "QtNetwork/QHostInfo"

#include "Logger/ConfiguratorFile.h"
#include "Utils/FilePath.h"

#include "AppBase.h"
#include "DisableAppNap.h"
#include "LoggerWindow.h"
#include "LogUtils.h"
#include "MainWindowBase.h"
#include "ManualWindow.h"
#include "PhantomCursorImaging.h"
#include "SCStyle.h"
#include "ToolWindowBase.h"
#include "Utils.h"
#include "WindowManager.h"

using namespace Utils;
using namespace SideCar::GUI;

static const char* const kLogConfigurationFile = "LogConfigurationFile";
static const char* const kObjectName = "ObjectName";
static const char* const kRootDocumentationPath = "/opt/sidecar/data/doc";

AppBase* AppBase::singleton_ = 0;

QString
AppBase::GetRootDocumentationPath()
{
    return QString(kRootDocumentationPath);
}

Logger::Log&
AppBase::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.AppBase");
    return log_;
}

AppBase*
AppBase::GetApp()
{
    return singleton_;
}

AppBase::AppBase(const QString& name, int& argc, char** argv)
    : QApplication(argc, argv), name_(name), docDir_(QDir(kRootDocumentationPath).absoluteFilePath(name_)),
      windowManager_(new WindowManager(this)), manualWindow_(0), loggerWindow_(0), lastConfigurationFile_(""),
      lastStyleSheetFile_(), config_(0), toolsMenuActions_(), loggingMenuActions_(), helpMenuActions_(),
      distanceUnits_("km"), distanceUnitsSuffix_(" km"), phantomCursor_(PhantomCursorImaging::InvalidCursor()),
      angularFormatType_(kDecimal), quitting_(false), disableAppNap_(new DisableAppNap)
{
    Logger::ProcLog log("AppBase", Log());
    LOGINFO << name << std::endl;
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kWarning);

    singleton_ = this;

#ifdef linux
    setStyle(new SCStyle);
#endif

    // QIcon icon(docDir_.filePath("screen.png"));
    // setWindowIcon(icon);

    // QPixmap pixmap(docDir_.filePath("screen.png"));
    // QSplashScreen* splash = new QSplashScreen(pixmap);
    // splash->show();
    // QTimer::singleShot(0, splash, SLOT(close()));

    connect(windowManager_, SIGNAL(activeMainWindowChanged(MainWindowBase*)),
            SIGNAL(activeMainWindowChanged(MainWindowBase*)));

    // We control when to quit based on how many managed windows there are. When the number goes to zero, we
    // begin the shutdown process.
    //
    setQuitOnLastWindowClosed(false);

    // Setup application name for QSettings file names. Append the the host name to deconflict when running the
    // application from different hosts but with the same NFS home directory.
    //
    setOrganizationName("bray.com");
    setApplicationName(QString("%1-%2").arg(name_) .arg(QHostInfo::localHostName()));

    // Create the QAction objects found in the stock menus. Must be done before the creation of any tool
    // windows.
    //
    makeLoggingMenuActions();
    makeHelpMenuActions();

    // Disable the SIGPIPE event sent out from socket drops.
    //
    struct sigaction noaction;
    ::memset(&noaction, 0, sizeof(noaction));
    noaction.sa_handler = SIG_IGN;
    ::sigaction(SIGPIPE, &noaction, 0);

    // Load the last Logging configuration file path, but do not load the file.
    //
    QSettings settings;
    lastConfigurationFile_ = settings.value(kLogConfigurationFile, "").toString();

    disableAppNap_->begin();
}

AppBase::~AppBase()
{
    Logger::ProcLog log("~AppBase", Log());
    LOGINFO << std::endl;
    if (config_) {
        delete config_;
    }
}

QString
AppBase::getDocumentationPath() const
{
    QDir dir(GetRootDocumentationPath());
    return dir.filePath(name_);
}

QMenu*
AppBase::makeApplicationMenu(QWidget* parent)
{
    QMenu* menu = new QMenu("App", parent);
    menu->setObjectName("ApplicationMenu");
    QAction* action = new QAction("Quit", parent);
    action->setShortcut(Qt::CTRL + Qt::Key_Q);
    action->setMenuRole(QAction::QuitRole);
    connect(action, SIGNAL(triggered()), SLOT(applicationQuit()));
    menu->addAction(action);
    return menu;
}

QMenu*
AppBase::makeToolsMenu(QWidget* parent)
{
    if (toolsMenuActions_.empty()) {
        return 0;
    }

    QMenu* menu = new QMenu("Tools", parent);
    menu->addActions(toolsMenuActions_);
    return menu;
}

QMenu*
AppBase::makeWindowMenu(QWidget* parent)
{
    return windowManager_->makeWindowMenu(parent);
}

QMenu*
AppBase::makeLoggingMenu(QWidget* parent)
{
    QMenu* menu = new QMenu("Logging", parent);
    menu->addActions(loggingMenuActions_);
    return menu;
}

QMenu*
AppBase::makeHelpMenu(QWidget* parent)
{
    QMenu* menu = new QMenu("Help", parent);
    menu->addActions(helpMenuActions_);
    return menu;
}

void
AppBase::restoreWindows()
{
    Logger::ProcLog log("restoreWindows", Log());
    LOGINFO << std::endl;

    // Restore all previously-saved MainWindowBase objects.
    //
    QSettings settings;
    int restored = 0;
    int count = settings.beginReadArray("Windows");
    LOGDEBUG << "restoring " << count << " windows" << std::endl;
    for (int index = 0; index < count; ++index) {
        settings.setArrayIndex(index);
        QString objectName = settings.value(kObjectName).toString();
        LOGDEBUG << "restoring window " << index << ' ' << objectName
                 << std::endl;
        MainWindowBase* window = makeAndInitializeNewMainWindow(objectName);
        if (window) {
            window->showAndRaise();
            ++restored;
        }
    }

    settings.endArray();

    // If this is the first time the application has been run, create a new main window now.
    //
    if (restored == 0) {
        LOGDEBUG << "none saved" << std::endl;
        MainWindowBase* window = makeAndInitializeNewMainWindow("");
        if (window) {
            window->showAndRaise();
        }
    }

    // Now restore all registered tool windows.
    //
    emit restoreToolWindows();
}

void
AppBase::setLogConfigurationFile(const QString& path)
{
    Logger::ProcLog log("setLogConfigurationFile", Log());
    LOGINFO << path << std::endl;

    // If the given path does not exist as a file, then just set to the empty string.
    //
    FilePath filePath(path.toStdString());
    if (!filePath.exists()) {
        filePath = "";
    }

    // Stop any previously-installed configuration.
    //
    lastConfigurationFile_ = QString::fromStdString(filePath.filePath());
    if (config_) {
        LOGWARNING << "stopping existing configuration file monitor"
                   << std::endl;
        config_->stopMonitor();
        delete config_;
        config_ = 0;
    }

    if (!filePath.empty()) {
        config_ = new Logger::ConfiguratorFile(path.toStdString());
        config_->startMonitor(5);
        LOGDEBUG << "applied logging configuration file: "
                 << path << std::endl;
    }
}

void
AppBase::loggingLoadConfigurationFile()
{
    Logger::ProcLog log("loggingLoadConfigurationFile", Log());
    LOGINFO << std::endl;

    QString path = QFileDialog::getOpenFileName(
            0, "Choose Logger configuration file", lastConfigurationFile_,
            "Config (*.cfg)");

    LOGDEBUG << path << std::endl;
    if (path.isEmpty()) {
        return;
    }

    setLogConfigurationFile(path);
}

void
AppBase::loadStyleSheet()
{
    QString path = QFileDialog::getOpenFileName(0, "Choose style sheet file", lastStyleSheetFile_, "CSS (*.css)");
    if (!path.isEmpty()) {
        lastStyleSheetFile_ = path;
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString css(file.readAll());
            setStyleSheet(css);
        }
        else {
            QMessageBox::critical(activeWindow(), "Failed Load",
                                  "Failed to read the style sheet file.");
        }
    }
}

void
AppBase::applicationQuit()
{
    Logger::ProcLog log("applicationQuit", Log());

    // If we are already in 'quit' sequence, don't continue.
    //
    if (quitting_) {
        return;
    }

    quitting_ = true;
    LOGWARNING << "*** USER QUITTING ***" << std::endl;

    // We are about to quit. Give all of the windows a chance to save their configurations. Record the logging
    // configuration. Broadcast a "shutdown" signal.
    //
    windowManager_->saveWindows();
    QSettings settings;
    settings.setValue(kLogConfigurationFile, lastConfigurationFile_);

    // Close all of the open windows. For MainWindowBase objects, this will invoke unmanageMainWindow(), which
    // will reduce the number of managed windows. A MainWindowBase object may reject a close via its
    // closeEvent() event handler. We detect when that occurs and stop the quit action.
    //
    closeAllWindows();
    if (getNumManagedWindows() != 0) {
        quitting_ = false;
        LOGWARNING << "*** NOT QUITTING *** - " << getNumManagedWindows() << std::endl;
        return;
    }

    // Tell interested parties that we are really shutting down.
    //
    emit shutdown();

    quit();
}

void
AppBase::addToolWindow(int index, ToolWindowBase* toolWindow)
{
    while (toolsMenuActions_.size() <= index) {
        toolsMenuActions_.append(0);
    }

    connect(this, SIGNAL(restoreToolWindows()), toolWindow, SLOT(applyInitialVisibility()));
    toolsMenuActions_[index] = toolWindow->getShowHideAction();
}

void
AppBase::makeLoggingMenuActions()
{
    loggerWindow_ = new LoggerWindow(Qt::CTRL + Qt::Key_D);
    connect(this, SIGNAL(restoreToolWindows()), loggerWindow_, SLOT(applyInitialVisibility()));

    QAction* action = MakeMenuAction("Load Configuration...", this, SLOT(loggingLoadConfigurationFile()),
                                     Qt::CTRL + Qt::Key_G);
    loggingMenuActions_.push_back(action);
    action = MakeMenuAction("Load Style Sheet...", this, SLOT(loadStyleSheet()), Qt::CTRL + Qt::Key_F10);
    loggingMenuActions_.push_back(action);
    loggingMenuActions_.push_back(loggerWindow_->getShowHideAction());
}

void
AppBase::makeHelpMenuActions()
{
    Logger::ProcLog log("makeHelpMenuActions", Log());

    helpMenuActions_.push_back(MakeMenuAction("About " + name_, this, SLOT(showAbout()), 0));

    QString manualPath = docDir_.filePath("manual.html");
    manualWindow_ = new ManualWindow(name_, manualPath);

    // Create an action to show the manual window.
    //
    QAction* action = new QAction(name_ + " Manual", this);
    action->setShortcut(Qt::Key_F1);
    connect(action, SIGNAL(triggered()), manualWindow_, SLOT(showAndRaise()));
    helpMenuActions_.push_back(action);

    action = new QAction(this);
    action->setSeparator(true);
    helpMenuActions_.push_back(action);
    helpMenuActions_.push_back(MakeMenuAction("Qt Version", this, SLOT(aboutQt()), 0));
}

void
AppBase::windowNew()
{
    Logger::ProcLog log("windowNew", Log());
    LOGINFO << std::endl;

    MainWindowBase* window = makeAndInitializeNewMainWindow("");
    LOGDEBUG << "new window: " << window << std::endl;

    if (window) {
        MainWindowBase* current = windowManager_->getActiveMainWindow();
        if (current) {
            LOGDEBUG << "offsetting window" << std::endl;
            window->resize(current->size());
            QPoint position(current->pos());
            position += QPoint(40, 40);
            window->move(position);
        }

        window->showAndRaise();
    }
}

MainWindowBase*
AppBase::makeAndInitializeNewMainWindow(const QString& type)
{
    MainWindowBase* window = makeNewMainWindow(type);
    if (window) {
        window->initialize();
    }

    return window;
}

void
AppBase::setDistanceUnits(const QString& units)
{
    distanceUnits_ = units;
    distanceUnitsSuffix_ = units;
    distanceUnitsSuffix_.prepend(' ');
    emit distanceUnitsChanged(distanceUnitsSuffix_);
}

QString
AppBase::getFormattedDistance(double value) const
{
    return GUI::DistanceToString(value, 2, 6).append(distanceUnitsSuffix_);
}

void
AppBase::setAngleFormatting(AngularFormatType type)
{
    angularFormatType_ = type;
    emit angleFormattingChanged(type);
}

QString
AppBase::getFormattedAngleRadians(double value) const
{
    return GUI::RadiansToString(value, angularFormatType_ == kDecimal);
}

QString
AppBase::getFormattedAngleDegrees(double value) const
{
    return GUI::DegreesToString(value, angularFormatType_ == kDecimal);
}

void
AppBase::showMessage(const QString& msg, int duration)
{
    if (windowManager_->getActiveMainWindow()) {
        windowManager_->getActiveMainWindow()->statusBar()->showMessage(msg, duration);
    }
}

void
AppBase::setPhantomCursor(const QPointF& pos)
{
    static Logger::ProcLog log("setPhantomCursor", Log());
    if (phantomCursor_ != pos && PhantomCursorImaging::IsValidCursor(pos)) {
        LOGINFO << pos << std::endl;
        phantomCursor_ = pos;
        emit phantomCursorChanged(pos);
    }
}

void
AppBase::showAbout()
{
    QString aboutText("");
    if (docDir_.exists("about.html")) {
        QString aboutPath = docDir_.filePath("about.html");
        QFile file(aboutPath);
        file.open(QIODevice::ReadOnly);
        aboutText.append(file.readAll());
    }
    else {
        aboutText = "Developer forgot to create an 'about.html' file for this application.";
    }

    QMessageBox::about(activeWindow(), name_, aboutText);
}

void
AppBase::updateToggleAction(bool state)
{
    UpdateToggleAction(qobject_cast<QAction*>(sender()), state);
}

int
AppBase::getNumManagedWindows() const
{
    return windowManager_->getNumManagedWindows();
}

MainWindowBase*
AppBase::getActiveMainWindow() const
{
    return windowManager_->getActiveMainWindow();
}

void
AppBase::manageWindow(QWidget* window, QMenu* windowMenu)
{
    windowManager_->manageWindow(window, windowMenu);
}

void
AppBase::unmanageWindow(QWidget* window)
{
    windowManager_->unmanageWindow(window);
}

bool
AppBase::isManagedWindow(QWidget* window) const
{
    return windowManager_->isManagedWindow(window);
}

QWidget*
AppBase::getManagedWindow(const QString& title) const
{
    return windowManager_->getManagedWindow(title);
}

void
AppBase::setVisibleWindowMenuNew(bool visible)
{
    windowManager_->setVisibleWindowMenuNew(visible);
}

void
AppBase::setVisibleWindowMenuClose(bool visible)
{
    windowManager_->setVisibleWindowMenuClose(visible);
}

void
AppBase::setVisibleWindowMenuMinimize(bool visible)
{
    windowManager_->setVisibleWindowMenuMinimize(visible);
}

void
AppBase::setVisibleWindowMenuMaximize(bool visible)
{
    windowManager_->setVisibleWindowMenuMaximize(visible);
}

void
AppBase::setVisibleWindowMenuFullScreen(bool visible)
{
    windowManager_->setVisibleWindowMenuFullScreen(visible);
}

void
AppBase::addWindowMenuActionTo(ToolBar* toolBar, int index) const
{
    windowManager_->addWindowMenuActionTo(toolBar, index);
}

bool
AppBase::event(QEvent* event)
{
    static Logger::ProcLog log("event", Log());
    LOGINFO << event->type() << std::endl;
    if (event->type() == QEvent::Close) {
        applicationQuit();
        return false;
    }

    return Super::event(event);
}

