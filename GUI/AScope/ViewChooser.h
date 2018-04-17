#ifndef SIDECAR_GUI_ASCOPE_VIEWCHOOSER_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_VIEWCHOOSER_H

#include "QtWidgets/QWidget"

class QComboBox;

namespace SideCar {
namespace GUI {
namespace AScope {

class ViewEditor;

class ViewChooser : public QWidget {
    Q_OBJECT
    using Super = QWidget;

public:
    ViewChooser(ViewEditor* viewEditor, QWidget* parent = 0);

private slots:

    void presetNamesChanged(const QStringList& names);

private:
    QComboBox* chooser_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

#endif
