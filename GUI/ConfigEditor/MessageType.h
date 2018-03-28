#ifndef SIDECAR_GUI_CONFIGEDITOR_MESSAGETYPE_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_MESSAGETYPE_H

#include "QtCore/QHash"
#include "QtCore/QString"

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

class MessageType {
public:
    static MessageType* Make(const QString& name);

    static MessageType* Find(const QString& name);

    const QString& getName() const { return name_; }

private:
    MessageType(const QString& name);

    QString name_;

    using HashTable = QHash<QString, MessageType*>;
    static HashTable hash_;
};

} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

#endif
