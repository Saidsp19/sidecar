#ifndef SIDECAR_GUI_CHANNELMENU_H // -*- C++ -*-
#define SIDECAR_GUI_CHANNELMENU_H

#include "QtCore/QList"
#include "QtGui/QMenu"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class ChannelSetting;

class ChannelMenu : public QMenu
{
    Q_OBJECT
    using Super = QMenu;
public:

    static Logger::Log& Log();

    ChannelMenu(ChannelSetting* setting, QWidget* parent = 0);

public slots:
    
    void setActive(const QString& channelName);

    void setChannelNames(const QList<QString>& names);

private slots:

    void itemSelected(QAction* action);

private:

    ChannelSetting* setting_;
    QList<QString> names_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
