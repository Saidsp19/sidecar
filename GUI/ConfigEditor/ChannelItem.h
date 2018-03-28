#ifndef SIDECAR_GUI_CONFIGEDITOR_CHANNEL_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_CHANNEL_H

#include "TreeItem.h"

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

class ChannelListItem;
class ConnectionItem;
class MessageType;

class ChannelItem : public TreeItem {
    Q_OBJECT
    using Super = TreeItem;

public:
    ChannelItem(ChannelListItem* parent, const QString& name, const QString& mappedName,
                const MessageType* messageType);

    Type getType() const { return kChannel; }

    ChannelListItem* getParent() const;

    ConnectionItem* getChild(int index) const;

    const QString& getMappedName() const { return mappedName_; }

    const MessageType* getMessageType() const { return messageType_; }

    const QString& getConnectionName() const { return connectionName_; }

public slots:

    void setMappedName(const QString& mappedName);

    void setMessageType(const MessageType* messageType);

private:
    QString mappedName_;
    const MessageType* messageType_;
    QString connectionName_;
};

} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

#endif
