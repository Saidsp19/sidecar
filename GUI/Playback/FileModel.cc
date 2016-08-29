#include "QtCore/QSettings"
#include "QtCore/QTimer"
#include "QtGui/QCloseEvent"
#include "QtGui/QMessageBox"
#include "QtGui/QProgressDialog"

#include "GUI/LogUtils.h"

#include "Emitter.h"
#include "FileModel.h"
#include "LoaderThread.h"
#include "MainWindow.h"

using namespace SideCar;
using namespace SideCar::GUI;
using namespace SideCar::GUI::Playback;

static const char* const kEmittings = "Emittings";

/** Derivation of QProgressDialog used by FileModel to indicate that a long load is in progress. The main focus
    of the method overrides is to prohibit the user from closing the dialog window before the load is finished.
*/
class FileModel::LoaderProgress : public QProgressDialog
{
    using Super = QProgressDialog;
public:
    
    /** Constructor.

        \param parent window to protect
    */
    LoaderProgress(QWidget* parent) : Super(parent) {}

    /** Keep the user from closing the dialog box via the ESC key.
     */
    void reject() {}
    
    /** Only perform done() when given an Accepted result code.

        \param r result code
    */
    void done(int r) { if (r != Accepted) return; Super::done(r); }
    
    /** Ignore a QCloseEvent unless our result code has been set to Accepted.

        \param event event to modify
    */
    void closeEvent(QCloseEvent* event)
	{
	    if (result() != Accepted) {
		event->ignore();
		return;
	    }
	    Super::closeEvent(event);
	}
};

Logger::Log&
FileModel::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("playback.FileModel");
    return log_;
}

FileModel::FileModel(MainWindow* parent)
    : QAbstractTableModel(parent), parent_(parent), emitters_(),
      startTime_(), endTime_(), suffix_(parent->getSuffix()), loaders_(),
      failures_(), loaderMeter_(0), dir_()
{
    connect(parent, SIGNAL(suffixChanged(const QString&)),
            SLOT(suffixChanged(const QString&)));
}

FileModel::~FileModel()
{
    qDeleteAll(emitters_);
    emitters_.clear();
}

void
FileModel::beginLoad(const QString& path)
{
    Logger::ProcLog log("beginLoad", Log());
    LOGINFO << "path: " << path << std::endl;

    parent_->statusBar()->clearMessage();

    // Obtain the names of the PRI files in the directory.
    //
    dir_.setPath(path);
    dir_.setFilter(QDir::Files | QDir::Readable);
    dir_.setNameFilters(QStringList("*.pri"));
    dir_.setSorting(QDir::Name);
    QFileInfoList files = dir_.entryInfoList();
    if (files.size() == 0) {
	QMessageBox::warning(parent_, "Failed",
                             "Failed to find any PRI files to load.",
                             QMessageBox::Ok);
	emit loadComplete();
	return;
    }

    // Clear out any existing Emitter objects.
    //
    if (! emitters_.empty()) {
	beginRemoveRows(QModelIndex(), 0, emitters_.size() - 1);
	qDeleteAll(emitters_);
	emitters_.clear();
	endRemoveRows();
    }

    // Initialize our min start time and max end time to very large values.
    //
    startTime_ = Time::TimeStamp::Max();
    endTime_ = Time::TimeStamp::Min();

    // Create a progress dialog that shows how many files we have left to process.
    //
    failures_.clear();

    loaderMeter_ = new LoaderProgress(parent_);
    loaderMeter_->setWindowTitle("Playback Loading");
    loaderMeter_->setLabelText(QString("Loading %1 file%2...")
                               .arg(files.size())
                               .arg(files.size() == 1 ? "" : "s"));
    loaderMeter_->setCancelButton(0);
    loaderMeter_->setRange(0, 0);
    loaderMeter_->setWindowModality(Qt::WindowModal);
    loaderMeter_->show();

    connect(loaderMeter_, SIGNAL(finished(int)),
            SLOT(finishedLoading()));

    // Now create the Emitter objects for the PRI files. We also create a LoaderThread object that will manage the
    // loading of the PRI files in a separate thread. For now, we just create them all in one shot and start them up.
    // If this is too hard on a system (it should be disk or network limited) then we can throttle the number of active
    // ones, activating new ones in the loaderFinished() method.
    //
    QSettings settings(dir_.absoluteFilePath("playbackSettings"));
    settings.beginGroup(kEmittings);

    beginInsertRows(QModelIndex(), 0, files.size() - 1);

    for (int index = 0; index < files.size(); ++index) {
	const QFileInfo& fileInfo(files.at(index));

	// Determine if this PRI file was emitting the last time it was loaded.
	//
	bool emitting = settings.value(fileInfo.baseName(), true).toBool();
	Emitter* emitter = new Emitter(parent_, fileInfo, index, emitting);
	connect(emitter, SIGNAL(loadPercentageUpdate(int)), this,
                SLOT(updateLoadPercentage(int)),
                Qt::QueuedConnection);
	connect(emitter, SIGNAL(subscriberCountChanged(int)), this,
                SLOT(updateSubscriberCount(int)));
	emitters_.append(emitter);
	LoaderThread* loader = new LoaderThread(emitter, fileInfo);
	loaders_.append(loader);
	connect(loader, SIGNAL(finished()), SLOT(loaderFinished()),
                Qt::QueuedConnection);
	loader->start();
    }

    endInsertRows();
}

void
FileModel::finishedLoading()
{
    Logger::ProcLog log("finishedLoading", Log());
    LOGINFO << std::endl;
    loaderMeter_->deleteLater();
    loaderMeter_ = 0;
}

void
FileModel::loaderFinished()
{
    Logger::ProcLog log("loaderFinished", Log());

    // Add loader object to the release pool.
    //
    LoaderThread* obj = qobject_cast<LoaderThread*>(sender());
    obj->deleteLater();

    // Remove from the list of unfinished loader objects
    //
    int index = loaders_.indexOf(obj);
    if (index == -1) {

	// !!! This should never happen.
	//
	LOGERROR << "*** missing LoaderThread object" << std::endl;
	return;
    }
    loaders_.removeAt(index);

    // If the Emitter object loaded successfully, connect it up. Otherwise, add its name to the list of
    // failures.
    //
    Emitter* emitter = obj->getEmitter();
    LOGDEBUG << "emitter: " << emitter->getName() << std::endl;

    if (emitter->isValid()) {
	connect(parent_, SIGNAL(addressChanged(const QString&)),
                emitter, SLOT(setAddress(const QString&)));
	connect(parent_, SIGNAL(suffixChanged(const QString&)),
                emitter, SLOT(setSuffix(const QString&)));
	if (emitter->getStartTime() < startTime_)
	    startTime_ = emitter->getStartTime();
	if (emitter->getEndTime() > endTime_)
	    endTime_ = emitter->getEndTime();
    }
    else {
	failures_.append(emitter->getName());
    }

    emit dataChanged(createIndex(index, 0),
                     createIndex(index, kDuration));

    if (! loaders_.empty()) {
	loaderMeter_->setLabelText(
	    QString("Loaded %1 - %2 remaining...")
	    .arg(emitters_.size() - loaders_.size())
	    .arg(loaders_.size()));
	return;
    }

    loaderMeter_->accept();

    // Notify others that our work is done.
    //
    emit loadComplete();

    // Post a message if there were any problems loading the Emitter objects.
    //
    if (failures_.size() == emitters_.size()) {
	QMessageBox::warning(parent_, "Failed",
                             "Failed to load any PRI files without errors.",
                             QMessageBox::Ok);
    }
    else if (! failures_.empty()) {
	if (failures_.size() == 1) {
	    QMessageBox::warning(parent_, "Load Failure",
                                 QString("Failed to load the PRI file '%1'")
                                 .arg(failures_[0]));
	}
	else {
	    QMessageBox::warning(parent_, "Load Failure",
                                 QString("<p>Failed to load the following "
                                         "PRI files:</p>%1")
                                 .arg(failures_.join(", ")));
	}
    }
}

void
FileModel::updateLoadPercentage(int row)
{
    QModelIndex idx(createIndex(row, kName)); 
    emit dataChanged(idx, idx);
}

void
FileModel::updateSubscriberCount(int row)
{
    QModelIndex idx(createIndex(row, kSubscriberCount)); 
    emit dataChanged(idx, idx);
}

int
FileModel::columnCount(const QModelIndex& parent) const
{
    return kNumColumns;
}

int
FileModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : emitters_.size();
}

QVariant
FileModel::data(const QModelIndex& pos, int role) const
{
    if (! pos.isValid() || pos.row() >= rowCount())
	return QVariant();

    QVariant value;
    Emitter* emitter = emitters_.at(pos.row());

    if (role == Qt::TextAlignmentRole) {
	int flags = Qt::AlignVCenter;
	if (pos.column() <= 1)
	    flags |= Qt::AlignHCenter;
	else
	    flags |= Qt::AlignRight;
	return flags;
    }
    else if (role == Qt::ForegroundRole) {
	if (pos.column() > 0 && ! emitter->isValid())
	    return Qt::darkRed;
    }

    switch (pos.column()) {
    case kEmitting:
	if (role == Qt::CheckStateRole || role == Qt::EditRole)
	    value = emitter->getEmitting() ? Qt::Checked : Qt::Unchecked;
	else if (role == Qt::ToolTipRole)
	    value = emitter->getEmitting() ?
		"Click to stop emitting data for this channel" :
		"Click to start emitting data for this channel";
                break;

    case kName:
	if (role == Qt::DisplayRole)
	    value = emitter->getName() + suffix_;
	else if (role == Qt::ToolTipRole)
	    value = QString("Port: %1").arg(emitter->getPort());
	break;

    case kStartTime:
	if (role == Qt::DisplayRole)
	    value = emitter->getFormattedStartTime();
	break;

    case kEndTime:
	if (role == Qt::DisplayRole)
	    value = emitter->getFormattedEndTime();
	break;

    case kDuration:
	if (role == Qt::DisplayRole)
	    value = emitter->getFormattedDuration();
	break;

    case kSubscriberCount:
	if (role == Qt::DisplayRole)
	    value = int(emitter->getSubscriberCount());
	break;

    default:
	break;
    }

    return value;
}

QVariant
FileModel::headerData(int section, Qt::Orientation orientation, int role)
    const
{
    QVariant value;
    if (role == Qt::DisplayRole) {
	if (orientation == Qt::Vertical) {
	    value = section + 1;
	}
	else {
	    switch (section) {
	    case kEmitting: value = "On"; break;
	    case kName: value = "Name"; break;
	    case kStartTime: value = "Start"; break;
	    case kEndTime: value = "End"; break;
	    case kDuration: value = "Duration"; break;
	    case kSubscriberCount: value = "#Subs"; break;
	    default: break;
	    }
	}
    }

    return value;
}

Qt::ItemFlags
FileModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags;
    if (! index.isValid())
	return flags;

    flags |= Qt::ItemIsEnabled;
    if (index.column() == kEmitting)
	flags |= Qt::ItemIsUserCheckable;

    return flags;
}

bool
FileModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (! index.isValid())
	return false;

    if (index.column() == kEmitting) {
	bool state = value.toBool();

	Emitter* emitter = emitters_[index.row()];
	emitter->setEmitting(state);
	emit dataChanged(index, index);

	QSettings settings(dir_.absoluteFilePath("playbackSettings"));
	settings.beginGroup(kEmittings);
	settings.setValue(emitter->getName(), state);

	return true;
    }

    return false;
}

void
FileModel::suffixChanged(const QString& suffix)
{
    suffix_ = suffix;
    emit dataChanged(createIndex(0, kName),
                     createIndex(rowCount() - 1, kName));
}
