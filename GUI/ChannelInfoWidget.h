#ifndef SIDECAR_GUI_CHANNELINFOWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_CHANNELINFOWIDGET_H

#include "QtCore/QStringList"
#include "QtGui/QWidget"

class QSignalMapper;

namespace Logger {
class Log;
}

namespace Ui {
class ChannelInfoWidget;
}
namespace SideCar {
namespace GUI {

class ChannelSetting;

class ChannelInfoWidget : public QWidget {
    Q_OBJECT
    using Super = QWidget;

public:
    ChannelInfoWidget(QWidget* parent = 0);

    void addChannel(const QString& tag, ChannelSetting* channelSetting);

private slots:

    void channelChanged(int index);

private:
    void setTagValue(int index, const ChannelSetting* channelSetting);

    void update();

    Ui::ChannelInfoWidget* gui_;
    QSignalMapper* mapper_;
    QStringList values_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
