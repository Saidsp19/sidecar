#include "QtCore/QFileInfo"
#include "QtCore/QSet"
#include "QtCore/QSettings"
#include "QtGui/QApplication"
#include "QtGui/QFileDialog"
#include "QtGui/QHeaderView"
#include "QtGui/QItemDelegate"
#include "QtGui/QItemSelectionModel"
#include "QtGui/QKeyEvent"
#include "QtGui/QMessageBox"

#include "GUI/LogUtils.h"
#include "GUI/modeltest.h"

#include "Cleaner.h"
#include "ConfigurationController.h"
#include "ConfigurationEditor.h"
#include "ConfigurationInfo.h"
#include "ConfigurationModel.h"
#include "ConfigurationSettings.h"
#include "MainWindow.h"
#include "RecordingController.h"
#include "RunnerItem.h"
#include "ServicesModel.h"
#include "UtilsWidget.h"

using namespace SideCar::GUI::Master;

static const char* const kLaunch = "Launch";
static const char* const kShutdown = "Shutdown";

Logger::Log&
ConfigurationController::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.ConfigurationController");
    return log_;
}

ConfigurationController::ConfigurationController(MainWindow& window, ConfigurationSettings& settings) :
    Super(&window), mainWindow_(window), settings_(settings), model_(0)
{
    model_ = new ConfigurationModel(this);
#ifdef __DEBUG__
    new ModelTest(model_, this);
#endif

    connect(model_, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
            SLOT(updateColumns(const QModelIndex&, const QModelIndex&)));

    QTableView* configurations = mainWindow_.configurations_;

    configurations->setModel(model_);
    configurations->installEventFilter(this);
    configurations->viewport()->installEventFilter(this);
    configurations->setAlternatingRowColors(true);
    connect(configurations, SIGNAL(doubleClicked(const QModelIndex&)), SLOT(editConfiguration(const QModelIndex&)));

    QItemSelectionModel* selectionModel = configurations->selectionModel();
    connect(selectionModel, SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
            SLOT(currentSelectionChanged(const QModelIndex&, const QModelIndex&)));

    QHeaderView* header = configurations->horizontalHeader();

#if 0
  header->setStyleSheet(
                        "QHeaderView::section {"
                        "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop: 0 "
                        "#616161, stop: 0.5 #505050, stop: 0.6 #434343, stop: 1 #656565);"
                        "color: #CCFFCC;"
                        "border: 1px solid #6c6c6c;"
                        "}");
#endif

    header->setResizeMode(QHeaderView::Fixed);

    header = configurations->verticalHeader();
    header->hide();
    header->setMinimumSectionSize(-1);
    header->setResizeMode(QHeaderView::ResizeToContents);

    connect(mainWindow_.configAdd_, SIGNAL(clicked()), SLOT(add()));
    connect(mainWindow_.actionConfigAdd_, SIGNAL(triggered()), SLOT(add()));

    connect(mainWindow_.configRemove_, SIGNAL(clicked()), SLOT(remove()));
    connect(mainWindow_.actionConfigRemove_, SIGNAL(triggered()), SLOT(remove()));

    connect(mainWindow_.configReload_, SIGNAL(clicked()), SLOT(reload()));
    connect(mainWindow_.actionConfigReload_, SIGNAL(triggered()), SLOT(reload()));

    connect(mainWindow_.configStartStop_, SIGNAL(clicked()), SLOT(launchShutdown()));
    connect(mainWindow_.actionConfigStartStop_, SIGNAL(triggered()), SLOT(launchShutdown()));

    connect(mainWindow_.getUtilsWidget()->cleanup_, SIGNAL(clicked()), SLOT(cleanup()));

    updateButtons();
}

QStringList
ConfigurationController::restore()
{
    QSettings settings;
    int lastSelected = settings.value("ConfigurationSelection", -1).toInt();

    QStringList failures;

    QStringList pastPaths = settings_.getConfigPaths();
    foreach (QString path, pastPaths) {
        ConfigurationInfo* obj = addPath(path, true);
        if (obj->hasError()) failures << path;
    }

    updatePaths();

    if (lastSelected != -1) {
        QItemSelectionModel* selectionModel = mainWindow_.configurations_->selectionModel();
        selectionModel->setCurrentIndex(model_->index(lastSelected, 0),
                                        QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
    }

    return failures;
}

void
ConfigurationController::currentSelectionChanged(const QModelIndex& current, const QModelIndex& previous)
{
    Logger::ProcLog log("currentSelectionChanged", Log());
    LOGINFO << "current: " << current.row() << " prev: " << previous.row() << std::endl;
    updateButtons();

    QSettings settings;
    settings.setValue("ConfigurationSelection", current.row());

    if (current.isValid()) emit selectedConfigurationChanged(model_->GetModelData(current));
}

ConfigurationInfo*
ConfigurationController::getCurrentConfiguration() const
{
    static Logger::ProcLog log("getCurrentConfiguration", Log());
    QItemSelectionModel* selectionModel = mainWindow_.configurations_->selectionModel();
    QModelIndex index = selectionModel->currentIndex();
    LOGDEBUG << "index: " << index.row() << '/' << index.column() << std::endl;
    return index.isValid() ? model_->getModelData(index.row()) : 0;
}

void
ConfigurationController::updateButtons()
{
    Logger::ProcLog log("updateButtons", Log());
    ConfigurationInfo* current = getCurrentConfiguration();

    LOGINFO << "current: " << current << " overrideCursor: " << QApplication::overrideCursor() << std::endl;

    mainWindow_.getUtilsWidget()->cleanup_->setEnabled(model_->rowCount() > 0 && !App::GetApp()->isRecording());

    if (current) {
        bool inProgress = current->isStartingUp() || current->isShuttingDown();
        LOGDEBUG << "inProgress: " << inProgress << std::endl;
        if (!inProgress && QApplication::overrideCursor()) {
            QApplication::restoreOverrideCursor();
            LOGDEBUG << "restoring override cursor" << std::endl;
        }

        bool enabled = !inProgress && !current->hasError();
        mainWindow_.configStartStop_->setEnabled(enabled);
        mainWindow_.actionConfigStartStop_->setEnabled(enabled);
        enabled = current->isNotRunning();
        mainWindow_.configReload_->setEnabled(enabled);
        mainWindow_.configRemove_->setEnabled(enabled);
        mainWindow_.actionConfigReload_->setEnabled(enabled);
        mainWindow_.actionConfigRemove_->setEnabled(enabled);

        if (enabled) {
            mainWindow_.configStartStop_->setText(kLaunch);
            mainWindow_.actionConfigStartStop_->setText(kLaunch);
            mainWindow_.configStartStop_->setToolTip("Start remote processing for the selected configuration");
            mainWindow_.actionConfigStartStop_->setToolTip("Start remote processing for the selected configuration");
        } else {
            mainWindow_.configStartStop_->setText(kShutdown);
            mainWindow_.configStartStop_->setToolTip("Stop remote processing for the selected configuration");
            mainWindow_.actionConfigStartStop_->setText(kShutdown);
            mainWindow_.actionConfigStartStop_->setToolTip("Stop remote processing for the selected configuration");
        }
    } else {
        mainWindow_.configStartStop_->setEnabled(false);
        mainWindow_.configReload_->setEnabled(false);
        mainWindow_.configRemove_->setEnabled(false);
        mainWindow_.actionConfigStartStop_->setEnabled(false);
        mainWindow_.actionConfigReload_->setEnabled(false);
        mainWindow_.actionConfigRemove_->setEnabled(false);
    }
}

void
ConfigurationController::add()
{
    // Get the path of the last element added.
    //
    QString prev = "/opt/sidecar/builds/latest/data/multicast.xml";
    if (model_->rowCount() != 0) {
        ConfigurationInfo* obj = getCurrentConfiguration();
        if (!obj) obj = model_->getModelData(model_->rowCount() - 1);
        prev = obj->getPath();
    }

    QString path = QFileDialog::getOpenFileName(0, "Choose a configuration file", prev, "Config (*.xml)", 0,
                                                QFileDialog::DontResolveSymlinks);

    if (path.isEmpty()) {
        showMessage("Load canceled", 5000);
        return;
    }

    ConfigurationInfo* obj = addPath(path, false);
    if (obj) { mainWindow_.getRecordingController().restore(obj->getRecordingsDirectory()); }

    updatePaths();
}

void
ConfigurationController::remove()
{
    model_->remove(model_->getRowOf(getCurrentConfiguration()));
    updatePaths();
    updateButtons();
}

void
ConfigurationController::reload()
{
    if (getCurrentConfiguration()->load(false)) {
        showMessage("Loaded", 5000);
        QMessageBox::information(
            qApp->activeWindow(), "Configuration Reloaded",
            QString("<p>Successfully reloaded configuration '%1'</p>").arg(getCurrentConfiguration()->getName()));
    } else {
        showMessage("Load failed", 5000);
    }
}

ConfigurationInfo*
ConfigurationController::addPath(const QString& path, bool restoring)
{
    ConfigurationInfo* obj = new ConfigurationInfo(this, model_, path);
    if (!obj->load(restoring)) {
        showMessage("Load failed", 5000);
    } else {
        showMessage("Loaded", 5000);
    }

    QModelIndex index = model_->add(obj);

    // Select the new entry for the benefit of the user.
    //
    mainWindow_.configurations_->setCurrentIndex(index);
    QItemSelectionModel* selectionModel = mainWindow_.configurations_->selectionModel();
    selectionModel->select(index, QItemSelectionModel::Clear);
    selectionModel->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    mainWindow_.configurations_->setFocus(Qt::OtherFocusReason);

#if 0
  QStringList runners = mainWindow_.getServicesModel()->getServiceNames(
                                                                        obj->getName());

  foreach (QString name, runners)
    obj->foundRunner(name);
#endif

    return obj;
}

void
ConfigurationController::launchShutdown()
{
    ConfigurationInfo* current = getCurrentConfiguration();
    Q_ASSERT(current);
    QStringList bits = mainWindow_.configStartStop_->text().split(' ');

    if (current->getStatus() == ConfigurationInfo::kNotRunning) {
        if (bits.size() == 2) {
            launchAll();
        } else {
            QApplication::setOverrideCursor(Qt::WaitCursor);
            if (current->startup()) {
                showMessage(current->getName() + " starting...");
            } else {
                QApplication::restoreOverrideCursor();
            }
        }
    } else {
        if (bits.size() == 2) {
            shutdownAll();
        } else {
            if (mainWindow_.shutdownConfiguration(current->getName())) { current->shuttingDown(); }
        }
    }
}

void
ConfigurationController::launchAll()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    for (int index = 0; index < model_->rowCount(); ++index) {
        ConfigurationInfo* configInfo = model_->getModelData(index);
        if (configInfo->isNotRunning()) { configInfo->startup(); }
    }
    QApplication::restoreOverrideCursor();
}

void
ConfigurationController::shutdownAll()
{
    if (App::GetApp()->isRecording()) {
        if (QMessageBox::question(&mainWindow_, "Shutdown Pending",
                                  QString("<p>A recording is in progress.</p>"
                                          "<p>Do you wish to stop recording and "
                                          "shutdown the SideCar processing "
                                          "streams?</p>"),
                                  QMessageBox::No | QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
            mainWindow_.showMessage("Shutdown canceled.");
            return;
        }

        mainWindow_.stopRecording();
    } else if (mainWindow_.isAnyActive()) {
        if (QMessageBox::question(&mainWindow_, "Shutdown Pending",
                                  QString("<p>Shut down all running configurations?</p>"
                                          "<p>Are you sure you want to shutdown all active "
                                          "SideCar processing streams?"),
                                  QMessageBox::No | QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
            mainWindow_.showMessage("Shutdown canceled.");
            return;
        }
    }

    for (int index = 0; index < model_->rowCount(); ++index) {
        ConfigurationInfo* configInfo = model_->getModelData(index);
        if (configInfo->isRunning()) {
            mainWindow_.forceShutdown(configInfo);
            configInfo->shuttingDown();
        }
    }
}

void
ConfigurationController::cleanup()
{
    QSet<QString> hostSet;
    for (int index = 0; index < model_->rowCount(); ++index) {
        ConfigurationInfo* configInfo = model_->getModelData(index);
        QStringList hosts = configInfo->getHostNames();
        foreach (QString host, hosts) {
            hostSet.insert(host);
        }
    }

    if (hostSet.size()) {
        QString text("<p>WARNING: All SideCar processes on the following "
                     "hosts are about to be forcibly terminated:</p>"
                     "<ul><li>");
        text.append(QStringList(hostSet.toList()).join("</li><li>"));
        text.append("</li></ul><p>Continue with this operation?</p>");
        int rc = QMessageBox::critical(qApp->activeWindow(), "Purge Remote Processes", text,
                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

        if (rc == QMessageBox::Yes) {
            mainWindow_.getRecordingController().reset();

            for (int index = 0; index < model_->rowCount(); ++index) {
                ConfigurationInfo* configInfo = model_->getModelData(index);
                configInfo->shuttingDown();
            }

            Cleaner* cleaner = new Cleaner(&mainWindow_, hostSet);
            connect(cleaner, SIGNAL(finished()), SLOT(cleanupDone()));
            cleaner->start();
        }
    }
}

void
ConfigurationController::cleanupDone()
{
    sender()->deleteLater();
    for (int index = 0; index < model_->rowCount(); ++index) {
        ConfigurationInfo* configInfo = model_->getModelData(index);
        configInfo->setStatus(ConfigurationInfo::kNotRunning);
    }
}

void
ConfigurationController::showMessage(const QString& text, int duration) const
{
    mainWindow_.showMessage(text, duration);
}

void
ConfigurationController::updatePaths()
{
    QStringList paths;
    for (int index = 0; index < model_->rowCount(); ++index) {
        ConfigurationInfo* obj = model_->getModelData(index);
        paths.append(obj->getPath());
    }
    settings_.setConfigPaths(paths);

    if (mainWindow_.getUtilsWidget()->find_->text() != "") mainWindow_.getUtilsWidget()->find_->setText("");

    updateStatus();
}

void
ConfigurationController::updateStatus()
{
    QStringList active;
    for (int index = 0; index < model_->rowCount(); ++index) {
        ConfigurationInfo* obj = model_->getModelData(index);
        if (!obj->isNotRunning()) { active.append(obj->getName()); }
    }

    updateButtons();

    emit activeConfigurationsChanged(active);
}

void
ConfigurationController::foundRunner(const RunnerItem* added)
{
    ConfigurationInfo* obj = model_->getModelData(added->getConfigName());
    if (obj) { obj->foundRunner(added); }
    mainWindow_.getUtilsWidget()->logs_->addLogInfo(added);
    mainWindow_.getUtilsWidget()->logs_->setEnabled(true);
    updateStatus();
}

void
ConfigurationController::lostRunner(const RunnerItem* removed)
{
    ConfigurationInfo* obj = model_->getModelData(removed->getConfigName());
    if (obj) obj->lostRunner(removed->getServiceName());

    updateStatus();
}

void
ConfigurationController::editConfiguration(const QModelIndex& index)
{
    if (index.column() > ConfigurationModel::kStatus) return;

    ConfigurationInfo* obj = model_->getModelData(index.row());
    if (obj && obj->isNotRunning()) {
        ConfigurationEditor* editor = obj->getEditor();
        editor->showAndRaise();
    }
}

bool
ConfigurationController::eventFilter(QObject* object, QEvent* event)
{
    if (object == mainWindow_.configurations_) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Backspace) {
                ConfigurationInfo* current = getCurrentConfiguration();
                if (current && current->isNotRunning()) {
                    remove();
                    return true;
                }
            }
        } else if (event->type() == QEvent::Resize) {
            adjustColumnSizes();
        }
    } else if (object == mainWindow_.configurations_->viewport()) {
        if (event->type() == QEvent::Resize) adjustColumnSizes();
    }
    return Super::eventFilter(object, event);
}

void
ConfigurationController::recordingStatusChanged(const QStringList& names, bool isRecording)
{
    model_->recordingStateChanged();
    foreach (QString name, names) {
        ConfigurationInfo* obj = model_->getModelData(name);
        if (obj && obj->isRunning()) obj->setRecordingState(isRecording);
    }
}

QStringList
ConfigurationController::getAllConfigurations() const
{
    QStringList found;
    for (int index = 0; index < model_->rowCount(); ++index) {
        ConfigurationInfo* obj = model_->getModelData(index);
        found.append(obj->getName());
    }

    return found;
}

QStringList
ConfigurationController::getViewableRunningConfigurations() const
{
    QStringList found;
    for (int index = 0; index < model_->rowCount(); ++index) {
        ConfigurationInfo* obj = model_->getModelData(index);
        if (obj->isViewable() && obj->isRunning()) found.append(obj->getName());
    }

    return found;
}

QStringList
ConfigurationController::getViewableConfigurations() const
{
    QStringList found;
    for (int index = 0; index < model_->rowCount(); ++index) {
        ConfigurationInfo* obj = model_->getModelData(index);
        if (obj->isViewable()) found.append(obj->getName());
    }

    return found;
}

QStringList
ConfigurationController::getRecordableConfigurations() const
{
    QStringList found;
    for (int index = 0; index < model_->rowCount(); ++index) {
        ConfigurationInfo* obj = model_->getModelData(index);
        if (obj->isRunning() && obj->canRecord()) found.append(obj->getName());
    }

    return found;
}

ConfigurationInfo*
ConfigurationController::getConfigurationInfo(const QString& name) const
{
    return model_->getModelData(name);
}

void
ConfigurationController::updateColumns(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    for (int column = topLeft.column(); column <= bottomRight.column(); ++column) {
        if (column == ConfigurationModel::kStatus) {
            updateStatus();
        } else if (column == ConfigurationModel::kViewable) {
            mainWindow_.filterRunners();
        } else if (column == ConfigurationModel::kRecordable) {
            mainWindow_.updateRecordingStartStop();
        }
    }

    adjustColumnSizes();
}

void
ConfigurationController::adjustColumnSizes()
{
    Logger::ProcLog log("adjustColumnSizes", Log());
    LOGINFO << "begin" << std::endl;

    // Approach: fetch the width of the view port, subtract the column width for all columns but the first one
    // (name), and allocate the remaining space to the first one if it is greater than its minimum width.
    //
    QTableView* configurations = mainWindow_.configurations_;
    configurations->resizeColumnsToContents();
    int minimum = configurations->columnWidth(0);
    int available(configurations->viewport()->width());
    for (int column = 1; column < model_->columnCount(); ++column) available -= configurations->columnWidth(column);
    if (available >= minimum) configurations->setColumnWidth(0, available);
}

QStringList
ConfigurationController::getInvalidRecordingDirectories() const
{
    QStringList invalid;
    for (int index = 0; index < model_->rowCount(); ++index) {
        ConfigurationInfo* obj = model_->getModelData(index);
        if (!obj->hasValidRecordingDirectory() && !invalid.contains(obj->getRecordingsDirectory()))
            invalid.append(obj->getRecordingsDirectory());
    }

    return invalid;
}

bool
ConfigurationController::isAnyRecording() const
{
    return model_->isAnyRecording();
}
