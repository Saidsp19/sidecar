#ifndef SIDECAR_GUI_PPIWIDGET_PATHSETTING_H // -*- C++ -*-
#define SIDECAR_GUI_PPIWIDGET_PATHSETTING_H

#include "QtWidgets/QLabel"
#include "QtWidgets/QPushButton"

#include "GUI/StringSetting.h"

namespace SideCar {
namespace GUI {

class PathSetting : public StringSetting {
    Q_OBJECT
public:
    PathSetting(PresetManager* mgr, QLabel* viewer, QPushButton* editor, const QString& prompt, const QString& types,
                bool global = false);

private slots:
    void choosePath();

private:
    QString prompt_;
    QString types_;
    QString last_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
