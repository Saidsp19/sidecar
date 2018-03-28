#ifndef SIDECAR_GUI_ASCOPE_VIEWEDITOR_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_VIEWEDITOR_H

#include "QtCore/QStringList"

#include "GUI/ToolWindowBase.h"

#include "ui_ViewEditor.h"

#include "ViewSettings.h"
#include "Visualizer.h"

namespace SideCar {
namespace GUI {
namespace AScope {

class DisplayView;

class ViewEditor : public ToolWindowBase, public Ui::ViewEditor {
    Q_OBJECT
    using Super = ToolWindowBase;

public:
    ViewEditor(int shortcut);

    ViewSettings makeView() const;

    QStringList getPresetNames() const;

    int getActivePresetIndex() const { return presets_->currentIndex(); }

    QAction* getHorizontalScaleToggleAction();

    QAction* getVerticalScaleToggleAction();

signals:

    void presetNamesChanged(const QStringList& names);

    void activePreset(int index);

public slots:

    void updateViewLimits();

    void activeDisplayViewChanged(DisplayView* displayView);

    void applyPreset(int index);

private slots:

    void on_presets__activated(int index);
    void on_fetch__clicked();
    void on_use__clicked();
    void on_update__clicked();
    void on_delete__clicked();
    void on_revert__clicked();
    void on_apply__clicked();

    void on_rangeMin__valueChanged(double value);
    void on_rangeMax__valueChanged(double value);
    void on_gateMin__valueChanged(int value);
    void on_gateMax__valueChanged(int value);

    void on_voltageMin__valueChanged(double value);
    void on_voltageMax__valueChanged(double value);
    void on_sampleMin__valueChanged(int value);
    void on_sampleMax__valueChanged(int value);

    void updateDistanceUnits(const QString& suffix);
    void shutdown();
    void updateButtons();
    void addPreset();

private:
    void showView(const ViewSettings& view);
    ViewSettings getView(int index) const;
    void addView(const QString& name, const ViewSettings& view);

    Visualizer* activeVisualizer_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

#endif
