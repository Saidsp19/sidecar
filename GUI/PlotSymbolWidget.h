#ifndef SIDECAR_GUI_PLOTSYMBOLWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_PLOTSYMBOLWIDGET_H

#include "QtWidgets/QComboBox"

namespace SideCar {
namespace GUI {

class TargetPlotImaging;

/** Derivation of a QComboBox that works with a TargetPlotImaging instance to show the available target plot
    symbols and the current symbol setting in use by the TargetPlotImaging instance.
*/
class PlotSymbolWidget : public QComboBox {
    Q_OBJECT
    using Super = QComboBox;

public:
    /** Constructor. Creates new widget that is not associated with any TargetPlotImaging instance.

        \param parent owner for auto-destruction
    */
    PlotSymbolWidget(QWidget* parent = 0) : Super(parent), settings_(0) {}

    /** Associate with a given TargetPlotImaging instance. When the plot color of the TargetPlotImaging instance
        changes we recreate our symbol icons to use the new color.

        \param settings TargetPlotImaging instance to associate with
    */
    void associateWith(TargetPlotImaging* settings);

public slots:

    /** Notification from the associated TargetPlotImaging that its plot color has changed. We obtain a new set
        of symbol icons drawn in the new color.
    */
    void colorChanged();

private:
    TargetPlotImaging* settings_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
