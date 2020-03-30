#ifndef SIDECAR_GUI_PPIDISPLAY_HISTORYWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_HISTORYWIDGET_H

#include "QtWidgets/QWidget"

class QBoxLayout;
class QCheckBox;
class QLabel;
class QSlider;
class QToolBar;

namespace SideCar {
namespace GUI {
namespace PPIDisplay {

class History;

class HistoryWidget : public QWidget {
    Q_OBJECT
    using Super = QWidget;

public:
    HistoryWidget(History* history, QToolBar* parent);

public slots:
    void changeOrientation(Qt::Orientation orientation);

private slots:
    void historyEnabledChanged(bool enabled);
    void historyRetainedCountChanged(int size);
    void historyCurrentViewChanged(int age);
    void historyCurrentViewAged(int age);

private:
    QString makeAgeText(int age);
    void updateOrientation(Qt::Orientation orientation);

    QCheckBox* enabled_;
    QSlider* slider_;
    History* history_;
    QBoxLayout* layout_;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

#endif
