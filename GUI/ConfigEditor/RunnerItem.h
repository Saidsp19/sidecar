#ifndef SIDECAR_GUI_CONFIGEDITOR_RUNNERITEM_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_RUNNERITEM_H

#include "TreeItem.h"

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

class ConfigurationItem;
class StreamItem;

class RunnerItem : public TreeItem {
    Q_OBJECT
    using Super = TreeItem;

public:
    RunnerItem(ConfigurationItem* parent, const QString& name, const QString& hostName,
               const QString& multicastAddress);

    ConfigurationItem* getParent() const;

    Type getType() const { return kRunner; }

    bool canAdopt(Type type) const { return type == kStream; }

    bool canReparent() const { return true; }

    const QString& getHostName() const { return hostName_; }

    const QString& getMulticastAddress() const { return multicastAddress_; }

public slots:

    void setHostName(const QString& hostName);

    void setMulticastAddress(const QString& multicastAddress);

private:
    QString hostName_;
    QString multicastAddress_;
};

} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

#endif
