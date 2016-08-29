#ifndef SIDECAR_GUI_QSLIDERSETTING_H // -*- C++ -*-
#define SIDECAR_GUI_QSLIDERSETTING_H

#include "QtCore/QList"
#include "QtGui/QSlider"

#include "GUI/IntSetting.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class QSliderSetting : public IntSetting
{
    Q_OBJECT
    using Super = IntSetting;
public:

    static Logger::Log& Log();

    QSliderSetting(PresetManager* mgr, QSlider* widget, bool global = false);

    void connectWidget(QSlider* widget);

    double getNormalizedValue() const;

private slots:

    void rangeChanged(int min, int max);

protected:
    
    virtual QString makeToolTip() const;

private:

    void valueUpdated();

    bool eventFilter(QObject* object, QEvent* event);

    QList<QSlider*> sliders_;
    int valueAtPress_;
    bool updatingRanges_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
