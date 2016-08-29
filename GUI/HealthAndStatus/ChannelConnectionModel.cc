#include "QtCore/QStringList"

#include "GUI/LogUtils.h"
#include "IO/ZeroconfRegistry.h"
#include "Messages/Video.h"

#include "ChannelConnection.h"
#include "ChannelConnectionModel.h"

using namespace SideCar;
using namespace SideCar::GUI::HealthAndStatus;

Logger::Log&
ChannelConnectionModel::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("hands.ChannelConnectionModel");
    return log_;
}

ChannelConnectionModel::ChannelConnectionModel(QObject* parent)
    : Super(parent), connections_(), browser_(0)
{
    browser_ = new ServiceBrowser(
	this, QString::fromStdString(
	    IO::ZeroconfTypes::Publisher::MakeZeroconfType(
		Messages::Video::GetMetaTypeInfo().getName())));
    connect(browser_,
            SIGNAL(availableServices(const ServiceEntryHash&)),
            SLOT(setAvailableServices(const ServiceEntryHash&)));
    browser_->start();
}

ChannelConnectionModel::~ChannelConnectionModel()
{
    delete browser_;
    foreach (ChannelConnection* connection, connections_) {
	connection->shutdown();
	connection->deleteLater();
    }
    connections_.clear();
}

void
ChannelConnectionModel::setAvailableServices(const ServiceEntryHash& services)
{
    static Logger::ProcLog log("setAvailableServices", Log());
    LOGINFO << services.count() << std::endl;

    QStringList unconnected;
    for (ServiceEntryHash::const_iterator pos = services.begin();
         pos != services.end(); ++pos) {
	ServiceEntry* serviceEntry = pos.value();
	LOGDEBUG << serviceEntry << ' ' << pos.key() << std::endl;
	ChannelConnection* w = findChannelConnection(pos.key());
	if (w) {
	    if (! w->isConnected()) {
		w->useServiceEntry(serviceEntry);
	    }
	}
	else {
	    unconnected.append(pos.key());
	}
    }

    qSort(unconnected);
    emit availableServicesChanged(unconnected);
}

QModelIndex
ChannelConnectionModel::index(int row, int column,
                              const QModelIndex& parent) const
{
    QModelIndex index(Super::index(row, column, parent));
    if (index != QModelIndex())
	index = createIndex(row, column, connections_[row]);
    return index;
}

int
ChannelConnectionModel::rowCount(const QModelIndex& parent) const
{
    return parent == QModelIndex() ? connections_.count() : 0;
}

QVariant
ChannelConnectionModel::data(const QModelIndex& index, int role) const
{
    if (! index.isValid()) return QVariant();
    if (role == Qt::SizeHintRole)
	return GetObject(index)->getMinimumSizeHint();
    return QVariant();
}

void
ChannelConnectionModel::moveUp(int row)
{
    Logger::ProcLog log("moveUp", Log());
    LOGINFO << "row: " << row << std::endl;
    swap(row, row - 1);
}

void
ChannelConnectionModel::moveDown(int row)
{
    Logger::ProcLog log("moveDown", Log());
    LOGINFO << "row: " << row << std::endl;
    swap(row, row + 1);
}

void
ChannelConnectionModel::swap(int row1, int row2)
{
    emit layoutAboutToBeChanged();
    QModelIndexList from;
    from.append(index(row1));
    from.append(index(row2));
    connections_.swap(row1, row2);
    QModelIndexList to;
    to.append(index(row2));
    to.append(index(row1));
    changePersistentIndexList(from, to);
    emit layoutChanged();
}

ChannelConnection*
ChannelConnectionModel::findChannelConnection(const QString& name) const
{
    foreach(ChannelConnection* obj, connections_) {
	if (obj->getName() == name)
	    return obj;
    }

    return 0;
}

QModelIndex
ChannelConnectionModel::add(ChannelConnection* channelConnection, int row)
{
    ServiceEntry* serviceEntry =
	browser_->getServiceEntry(channelConnection->getName());

    if (serviceEntry)
	channelConnection->useServiceEntry(serviceEntry);

    beginInsertRows(QModelIndex(), row, row);
    connections_.insert(row, channelConnection);
    endInsertRows();

    return index(row);
}

bool
ChannelConnectionModel::removeRows(int row, int count,
                                   const QModelIndex& parent)
{
    beginRemoveRows(parent, row, row + count - 1);

    while (count-- > 0) {
	ChannelConnection* w = connections_.takeAt(row);
	w->shutdown();
	w->deleteLater();
    }

    endRemoveRows();
    return true;
}

bool
ChannelConnectionModel::hasChannel(const QString& name) const
{
    foreach (ChannelConnection* obj, connections_) {
	if (obj->getName() == name)
	    return true;
    }

    return false;
}
