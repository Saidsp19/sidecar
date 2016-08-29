#ifndef SIDECAR_GUI_SPECTRUM_VIEWEDITOR_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_VIEWEDITOR_H

#include "QtCore/QStringList"

#include "GUI/ToolWindowBase.h"

#include "ui_ViewEditor.h"

#include "ViewSettings.h"

namespace SideCar {
namespace GUI {
namespace Spectrum {

class Settings;
class SpectrumWidget;

class ViewEditor : public ToolWindowBase, public Ui::ViewEditor
{
    Q_OBJECT
    using Super = ToolWindowBase;
public:

    ViewEditor(int shortcut);

    ViewSettings makeView() const;

    QStringList getPresetNames() const;

    int getActivePresetIndex() const { return presets_->currentIndex(); }

signals:

    void presetNamesChanged(const QStringList& names);

    void activePreset(int index);

public slots:

    void updateViewLimits();

    void setSpectrumWidget(SpectrumWidget* spectrumWidget);

    void applyPreset(int index);

private slots:

    void on_presets__activated(int index);
    void on_use__clicked();
    void on_update__clicked();
    void on_delete__clicked();
    void on_revert__clicked();
    void on_apply__clicked();

    void on_xMin__valueChanged(double value);
    void on_xMax__valueChanged(double value);
    void on_yMin__valueChanged(double value);
    void on_yMax__valueChanged(double value);
    void on_xMinScale__currentIndexChanged(int value);
    void on_xMaxScale__currentIndexChanged(int value);

    void shutdown();

    void updateButtons();

    void addPreset();

private:

    void showView(const ViewSettings& view); 

    ViewSettings getView(int index) const;

    void addView(const QString& name, const ViewSettings& view);

    SpectrumWidget* spectrumWidget_;
    double xMinScaleValue_;
    double xMaxScaleValue_;
    double xMinValue_;
    double xMaxValue_;
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

#endif
