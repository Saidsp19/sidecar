#ifndef SIDECAR_GUI_CONFIGEDITOR_CONNECTION_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_CONNECTION_H

#include "QtCore/QObject"

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

class ChannelItem;

class Connection : public QObject
{
    Q_OBJECT
    using Super = QObject;
public:

    Connection(ChannelItem* from, ChannelItem* to);

    ~Connection();

private:
    ChannelItem* from_;
    ChannelItem* to_;
    int fromIndex_;
    int toIndex_;
};

} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

#endif
