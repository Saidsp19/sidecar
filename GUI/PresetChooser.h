#ifndef SIDECAR_GUI_PRESETCHOOSER_H // -*- C++ -*-
#define SIDECAR_GUI_PRESETCHOOSER_H

#include "QtWidgets/QComboBox"

namespace Logger {
class Log;
}
namespace SideCar {
namespace GUI {

class PresetManager;

class PresetChooser : public QComboBox {
    Q_OBJECT
    using Super = QComboBox;

public:
    static Logger::Log& Log();

    PresetChooser(PresetManager* presetManager, QWidget* parent = 0);

private slots:

    void activePresetChanged(int index);

    void presetNamesChanged(const QStringList& names);

    void setDirtyState(int index, bool state);

private:
    void mousePressEvent(QMouseEvent* event);

    PresetManager* presetManager_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
