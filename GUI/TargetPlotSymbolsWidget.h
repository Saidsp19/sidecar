#ifndef SIDECAR_GUI_TARGETPLOTSYMBOLSWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_TARGETPLOTSYMBOLSWIDGET_H

#include "QtGui/QWidget"

namespace Ui {
class TargetPlotSymbolsWidget;
}
namespace SideCar {
namespace GUI {

class TargetPlotImaging;

/** Widget that shows the current plot symbols for the extractions (E), range truths (T) and user pick (P)
    imaging sources.
*/
class TargetPlotSymbolsWidget : public QWidget {
    Q_OBJECT
    using Super = QWidget;

public:
    /** Constructor.

        \param parent parent for auto-destruction
    */
    TargetPlotSymbolsWidget(QWidget* parent = 0);

    /** Associate the extractions symbol QComboBox with the given TargetPlotImaging instance.

        \param setting TargetPlotImaging instance to associate with
    */
    void connectExtractionsSymbolType(TargetPlotImaging* setting);

    /** Associate the range truths symbol QComboBox with the given TargetPlotImaging instance.

        \param setting TargetPlotImaging instance to associate with
    */
    void connectRangeTruthsSymbolType(TargetPlotImaging* setting);

    /** Associate the user picks symbol QComboBox with the given TargetPlotImaging instance.

        \param setting TargetPlotImaging instance to associate with
    */
    void connectBugPlotsSymbolType(TargetPlotImaging* setting);

private:
    Ui::TargetPlotSymbolsWidget* gui_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
