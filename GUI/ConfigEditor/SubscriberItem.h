#ifndef SIDECAR_GUI_CONFIGEDITOR_SUBSCRIBERITEM_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_SUBSCRIBERITEM_H

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

class SubscriberItem : public TreeItem
{
    Q_OBJECT
public:

    SubscriberItem(const QString& name, TreeItem* parent);
    
private:
};


} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

#endif
