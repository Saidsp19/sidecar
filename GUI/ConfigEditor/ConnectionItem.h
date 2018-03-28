#ifndef SIDECAR_GUI_CONFIGEDITOR_CONNECTIONITEM_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_CONNECTIONITEM_H

#include "TreeItem.h"

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

class ChannelItem;

class ConnectionItem : public TreeItem {
    Q_OBJECT
    using Super = TreeItem;

public:
    ConnectionItem(ChannelItem* parent, ChannelItem* to);

    Type getType() const { return kConnection; }

    ChannelItem* getParent() const;

    ChannelItem* getFrom() const { return getParent(); }

    ChannelItem* getTo() const { return to_; }

private:
    ChannelItem* to_;
};

} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

#endif
