#ifndef SIDECAR_GUI_LOGGERVIEW_H // -*- C++ -*-
#define SIDECAR_GUI_LOGGERVIEW_H

#include "QtGui/QTreeView"

namespace SideCar {
namespace GUI {

class LoggerView : public QTreeView {
    Q_OBJECT
    using Super = QTreeView;

public:
    LoggerView(QWidget* parent = 0);

private slots:

    void saveExpansionState(const QModelIndex& index);

private:
    void rowsInserted(const QModelIndex& parent, int start, int end);
    bool getWasExpanded(const QModelIndex& index) const;
};

} // end namespace GUI
} // end namespace SideCar

#endif
