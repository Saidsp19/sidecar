#include "QtCore/QSettings"
#include "QtGUI/QGuiApplication"

#include "GUI/LogUtils.h"
#include "GUI/ServiceEntry.h"
#include "GUI/Utils.h"

#include "IO/ZeroconfRegistry.h"
#include "Messages/Video.h"
#include "Utils/Utils.h"

#include "App.h"
#include "ChannelConnection.h"
#include "ChannelConnectionModel.h"
#include "ChannelConnectionWindow.h"
#include "Configuration.h"
#include "DefaultViewSettings.h"
#include "PeakBarSettings.h"
#include "VideoChannel.h"
#include "Visualizer.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::AScope;
using namespace SideCar::Messages;
using namespace Utils;

static const char* const kVideoChannels = "VideoChannels";

std::ostream&
operator<<(std::ostream& os, const QSet<QString>& container)
{
    foreach (const QString& value, container)
        os << value << ',';
    return os;
}

Logger::Log&
ChannelConnectionModel::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ascope.ChannelConnectionModel");
    return log_;
}

ChannelConnectionModel::ChannelConnectionModel(ChannelConnectionWindow* par) :
    QAbstractTableModel(par), peakBarSettings_(App::GetApp()->getPeakBarSettings()),
    history_(App::GetApp()->getHistory()), visualizer_(0), browser_(0), channels_(), available_()
{
    Logger::ProcLog log("ChannelConnectionModel", Log());
    LOGINFO << std::endl;

    connect(&peakBarSettings_, SIGNAL(enabledChanged(bool)), SLOT(peakBarSettingsEnabledChanged(bool)));

    browser_ = new ServiceBrowser(this, QString::fromStdString(IO::ZeroconfTypes::Publisher::MakeZeroconfType(
                                            Messages::Video::GetMetaTypeInfo().getName())));

    connect(browser_, SIGNAL(availableServices(const ServiceEntryHash&)),
            SLOT(setAvailableServices(const ServiceEntryHash&)));

    browser_->start();
}

ChannelConnectionModel::~ChannelConnectionModel()
{
    for (VideoChannelHash::iterator pos = channels_.begin(); pos != channels_.end(); ++pos) { pos.value()->shutdown(); }
}

void
ChannelConnectionModel::setVisualizer(Visualizer* visualizer)
{
    static Logger::ProcLog log("setVisualizer", Log());
    LOGINFO << "old: " << visualizer_ << " new: " << visualizer << std::endl;

    // CAREFUL! The rowCount() method relies on visualizer_ to get its value. Make sure we set visualizer_ only
    // after we are done with the old value.
    //

    // Fetch the number of rows in the model for the current visualizer_ value.
    //
    int oldRows = rowCount();
    if (oldRows) {
        // Tell views that these rows are going away.
        //
        beginRemoveRows(QModelIndex(), 0, oldRows - 1);

        // Zap our visualizer_ value so any future rowCount() calls will return zero.
        //
        visualizer_ = 0;
        beginResetModel();
        endResetModel();

        endRemoveRows();
    }

    // Now assign the new visualzer_ value, and fetch the number of entries it has.
    //
    int newRows = visualizer ? visualizer->getNumChannelConnections() : 0;
    if (newRows) beginInsertRows(QModelIndex(), 0, newRows - 1);

    visualizer_ = visualizer;
    if (newRows) endInsertRows();

    QSet<QString> unconnected = available_;
    LOGDEBUG << "available: " << available_ << std::endl;

    if (newRows) unconnected -= visualizer_->getConnectionNames();

    LOGDEBUG << "unconnected: " << unconnected << std::endl;
    QStringList tmp = unconnected.toList();
    qSort(tmp);
    LOGDEBUG << "tmp: " << tmp << std::endl;

    getParent()->setUnconnected(tmp);
}

ChannelConnectionWindow*
ChannelConnectionModel::getParent() const
{
    return static_cast<ChannelConnectionWindow*>(QObject::parent());
}

ChannelConnection*
ChannelConnectionModel::getChannelConnection(int row) const
{
    return visualizer_ ? visualizer_->getChannelConnection(row) : 0;
}

void
ChannelConnectionModel::addVideoChannel(VideoChannel* channel)
{
    static Logger::ProcLog log("addVideoChannel", Log());
    LOGINFO << channel << std::endl;

    channels_.insert(channel->getName(), channel);

    connect(channel, SIGNAL(connected()), SLOT(channelConnectionChanged()));

    connect(channel, SIGNAL(disconnected()), SLOT(channelConnectionChanged()));

    ServiceEntry* service = browser_->getServiceEntry(channel->getName());
    if (service) channel->useServiceEntry(service);
}

VideoChannel*
ChannelConnectionModel::findVideoChannel(const QString& name) const
{
    static Logger::ProcLog log("findVideoChannel", Log());
    LOGINFO << name << std::endl;
    return channels_.value(name, 0);
}

int
ChannelConnectionModel::findChannelConnectionRow(VideoChannel* channel) const
{
    return visualizer_ ? visualizer_->findVideoChannel(channel) : -1;
}

int
ChannelConnectionModel::findChannelConnectionRow(const QString& name) const
{
    return findChannelConnectionRow(findVideoChannel(name));
}

VideoChannel*
ChannelConnectionModel::getVideoChannel(const QString& channelName)
{
    static Logger::ProcLog log("getVideoChannel", Log());
    LOGINFO << channelName << std::endl;
    VideoChannel* channel = findVideoChannel(channelName);
    if (!channel) {
        QSettings settings;
        settings.beginGroup(kVideoChannels);
        channel = new VideoChannel(history_, channelName);
        channel->restoreFromSettings(settings);
        addVideoChannel(channel);
    }
    return channel;
}

bool
ChannelConnectionModel::makeConnection(const QString& channelName)
{
    static Logger::ProcLog log("makeConnection", Log());
    LOGINFO << channelName << std::endl;
    if (!visualizer_) {
        LOGERROR << "NULL visualizer" << std::endl;
        return false;
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    VideoChannel* channel = getVideoChannel(channelName);
    DefaultViewSettings& settings(App::GetApp()->getDefaultViewSettings());
    visualizer_->addVideoChannel(*channel, settings.getVisible(), settings.getShowPeakBars());
    endInsertRows();

    return true;
}

ServiceEntry*
ChannelConnectionModel::removeConnection(int index)
{
    static Logger::ProcLog log("removeConnection", Log());
    LOGINFO << index << std::endl;
    beginRemoveRows(QModelIndex(), index, index);
    VideoChannel* channel = visualizer_->removeChannelConnection(index);
    endRemoveRows();
    QString channelName(channel->getName());
    if (!channel->isDisplayed()) {
        channels_.remove(channelName);
        delete channel;
    }

    return browser_->getServiceEntry(channelName);
}

int
ChannelConnectionModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : kNumColumns;
}

int
ChannelConnectionModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : (visualizer_ ? visualizer_->getNumChannelConnections() : 0);
}

QVariant
ChannelConnectionModel::data(const QModelIndex& pos, int role) const
{
    auto palette = QGuiApplication::palette();

    static Logger::ProcLog log("data", Log());
    if (!visualizer_ || !pos.isValid() || pos.row() >= rowCount()) return QVariant();

    ChannelConnection* connection = getChannelConnection(pos.row());
    const VideoChannel& channel = connection->getChannel();

    QVariant value;
    if (role == Qt::TextAlignmentRole) {
        if (pos.column() > kColor) return int(Qt::AlignRight | Qt::AlignVCenter);
        return value;
    } else if (role == Qt::ForegroundRole) {
        return QVariant(channel.isConnected() ? palette.windowText().color() : WarningRedColor());
    }

    switch (pos.column()) {
    case kName:

        // The values returned for the kName column are either the name text or the display font. The font
        // italicized if the channel is not connected.
        //
        switch (role) {
        case Qt::DisplayRole: value = channel.getName(); break;
        case Qt::ToolTipRole: value = "Double-click to change sample min/max values"; break;
        default: break;
        }
        break;

    case kColor:

        // The value returned for the kColor column are the QColor value stored in the configuration.
        //
        if (role == Qt::BackgroundColorRole || role == Qt::EditRole)
            value = channel.getColor();
        else if (role == Qt::ToolTipRole)
            value = "Double-click to change the color";
        break;

    case kSampleMin:
        if (role == Qt::DisplayRole)
            value = channel.getSampleMin();
        else if (role == Qt::ToolTipRole)
            value = "Double-click to change sample min/max values";
        break;

    case kSampleMax:
        if (role == Qt::DisplayRole)
            value = channel.getSampleMax();
        else if (role == Qt::ToolTipRole)
            value = "Double-click to change sample min/max values";
        break;

    case kVoltageMin:
        if (role == Qt::DisplayRole)
            value = channel.getVoltageMin();
        else if (role == Qt::ToolTipRole)
            value = "Double-click to change sample min/max values";
        break;

    case kVoltageMax:
        if (role == Qt::DisplayRole)
            value = channel.getVoltageMax();
        else if (role == Qt::ToolTipRole)
            value = "Double-click to change sample min/max values";
        break;

    case kVisible:

        // The visibility state is represented as a checkbox which is either checked or unchecked.
        //
        if (role == Qt::CheckStateRole || role == Qt::EditRole)
            value = connection->isVisible() ? Qt::Checked : Qt::Unchecked;
        else if (role == Qt::ToolTipRole)
            value = connection->isVisible() ? "Click to stop plotting channel data"
                                            : "Click to begin plotting channel data";

        break;

    case kFrozen:

        // The frozen state is represented as a checkbox which is either checked or unchecked.
        //
        if (role == Qt::CheckStateRole || role == Qt::EditRole)
            value = connection->isFrozen() ? Qt::Checked : Qt::Unchecked;
        else if (role == Qt::ToolTipRole)
            value = connection->isFrozen() ? "Click to resume updating channel data" : "Click to freeze channel data";
        break;

    case kShowPeakBars:

        // The showPeakBars state is represented as a checkbox which is either checked or unchecked.
        //
        if (role == Qt::CheckStateRole || role == Qt::EditRole)
            value = connection->isShowingPeakBars() ? Qt::Checked : Qt::Unchecked;
        else if (role == Qt::ToolTipRole)
            value = connection->isShowingPeakBars() ? "Click to hide peak bars" : "Click to show peak bars";
        break;

    case kDisplayCount:
        if (role == Qt::DisplayRole) value = uint(channel.getDisplayCount());
        break;

    default: break;
    }

    return value;
}

QVariant
ChannelConnectionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) return QVariant();
    if (orientation == Qt::Vertical) return section + 1;
    switch (section) {
    case kName: return "Channel";
    case kSampleMin: return "CMin";
    case kSampleMax: return "CMax";
    case kVoltageMin: return "VMin";
    case kVoltageMax: return "VMax";
    case kVisible: return "Visible";
    case kFrozen: return "Frozen";
    case kShowPeakBars: return "Peaks";
    case kColor: return "Color";
    case kDisplayCount: return "Views";
    default: break;
    }
    return QVariant();
}

Qt::ItemFlags
ChannelConnectionModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) return Super::flags(index);

    Qt::ItemFlags flags(Qt::ItemIsEnabled);

    if (index.isValid()) {
        switch (index.column()) {
        case kName: flags |= Qt::ItemIsSelectable; break;

        case kVisible:
        case kFrozen: flags |= Qt::ItemIsUserCheckable; break;

        case kShowPeakBars:
            if (!peakBarSettings_.isEnabled()) flags = Qt::ItemFlags();
            flags |= Qt::ItemIsUserCheckable;
            break;

        default: break;
        }
    }

    return flags;
}

bool
ChannelConnectionModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    static Logger::ProcLog log("setData", Log());
    if (!index.isValid()) return false;

    LOGINFO << index.row() << '/' << index.column() << std::endl;

    Q_ASSERT(visualizer_);

    ChannelConnection* connection = visualizer_->getChannelConnection(index.row());

    switch (index.column()) {
    case kVisible:
        LOGDEBUG << "changing visibility" << std::endl;
        connection->setVisible(value.toBool());
        break;

    case kFrozen:
        LOGDEBUG << "changing freeze state" << std::endl;
        connection->setFrozen(value.toBool());
        break;

    case kShowPeakBars:
        LOGDEBUG << "changing peak bars state" << std::endl;
        connection->setShowPeakBars(value.toBool());
        break;

    case kColor:
        LOGDEBUG << "changing color" << std::endl;
        connection->setColor(value.value<QColor>());
        break;

    case kSampleMin:
    case kSampleMax:
    case kVoltageMin:
    case kVoltageMax: break;

    default: return false;
    }

    emit dataChanged(index, index);
    return true;
}

void
ChannelConnectionModel::channelConnectionChanged()
{
    static Logger::ProcLog log("channelConnectionChanged", Log());
    VideoChannel* channel = dynamic_cast<VideoChannel*>(sender());
    LOGINFO << channel->getName() << ' ' << channel->isConnected() << std::endl;
    int row = findChannelConnectionRow(channel);
    LOGDEBUG << row << std::endl;
    if (row != -1) { emit dataChanged(createIndex(row, kName), createIndex(row, kDisplayCount)); }
}

void
ChannelConnectionModel::setAvailableServices(const ServiceEntryHash& services)
{
    static Logger::ProcLog log("setAvailableServices", Log());
    LOGINFO << services.count() << std::endl;

    available_.clear();

    for (ServiceEntryHash::const_iterator pos = services.begin(); pos != services.end(); ++pos) {
        available_.insert(pos.key());
        ServiceEntry* serviceEntry = pos.value();
        LOGDEBUG << serviceEntry << ' ' << pos.key() << std::endl;
        VideoChannel* channel = findVideoChannel(pos.key());
        if (channel) {
            if (!channel->isConnected()) { channel->useServiceEntry(serviceEntry); }
        }
    }

    setVisualizer(visualizer_);
}

void
ChannelConnectionModel::moveUp(int row)
{
    if (!visualizer_) return;
    emit layoutAboutToBeChanged();
    visualizer_->lowerChannelConnection(row);
    emit layoutChanged();
}

void
ChannelConnectionModel::moveDown(int row)
{
    if (!visualizer_) return;
    emit layoutAboutToBeChanged();
    visualizer_->raiseChannelConnection(row);
    emit layoutChanged();
}

void
ChannelConnectionModel::saveVideoChannelSettings()
{
    QSettings settings;
    settings.beginGroup(kVideoChannels);
    for (VideoChannelHash::const_iterator pos = channels_.begin(); pos != channels_.end(); ++pos) {
        pos.value()->saveToSettings(settings);
    }
}

void
ChannelConnectionModel::peakBarSettingsEnabledChanged(bool)
{
    Logger::ProcLog log("peakBarSettingsEnabledChanged", Log());
    LOGINFO << std::endl;
    emit dataChanged(createIndex(0, kShowPeakBars), createIndex(rowCount() - 1, kShowPeakBars));
}
