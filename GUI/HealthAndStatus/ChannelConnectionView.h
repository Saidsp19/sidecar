#ifndef SIDECAR_GUI_HEALTHANDSTATUS_CHANNELCONNECTIONVIEW_H // -*- C++ -*-
#define SIDECAR_GUI_HEALTHANDSTATUS_CHANNELCONNECTIONVIEW_H

#include "QtWidgets/QListView"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace HealthAndStatus {

class ChannelConnectionView : public QListView {
    Q_OBJECT
    using Super = QListView;

public:
    ChannelConnectionView(QWidget* parent = 0) : Super(parent) {}

signals:

    void clearAll();

    void clearDrops();

private:
    void keyPressEvent(QKeyEvent* event);
};

} // end namespace HealthAndStatus
} // end namespace GUI
} // end namespace SideCar

#endif
