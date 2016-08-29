#ifndef SIDECAR_GUI_ESSCOPE_VIEWEDITOR_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_VIEWEDITOR_H

#include "QtCore/QMetaType"
#include "QtCore/QStringList"

#include "GUI/ToolWindowBase.h"

#include "ui_ViewEditor.h"

class QSettings;

namespace SideCar {
namespace GUI {
namespace ESScope {

class ViewSettings;

struct ViewPreset {

    ViewPreset() {}

    ViewPreset(QSettings& settings);

    ViewPreset(double a1, double a2, double b1, double b2,
               double a3, double a4, double r1, double r2)
	: alpha1Min(a1), alpha1Max(a2), betaMin(b1), betaMax(b2),
	  alpha2Min(a3), alpha2Max(a4), rangeMin(r1), rangeMax(r2) {}

    void restoreFromSettings(QSettings& settings);

    void saveToSettings(QSettings& settings) const;

    bool operator==(const ViewPreset& rhs) const
	{ return alpha1Min == rhs.alpha1Min && alpha1Max == rhs.alpha1Max &&
		betaMin == rhs.betaMin && betaMax == rhs.betaMax &&
		alpha2Min == rhs.alpha2Min && alpha2Max == rhs.alpha2Max &&
		rangeMin == rhs.rangeMin && rangeMax == rhs.rangeMax; }

    bool operator!=(const ViewPreset& rhs) const
	{ return ! operator==(rhs); }

    double alpha1Min, alpha1Max;
    double betaMin, betaMax;
    double alpha2Min, alpha2Max;
    double rangeMin, rangeMax;

    static int const kMetaTypeId;
};

class ViewEditor : public ToolWindowBase, public Ui::ViewEditor
{
    Q_OBJECT
    using Super = ToolWindowBase;
public:

    ViewEditor(int shortcut);

    QStringList getPresetNames() const;

    int getActivePresetIndex() const { return presets_->currentIndex(); }

    void setViewSettings(ViewSettings* alphaBetaViewSettings,
                         ViewSettings* alphaRangeViewSettings);

signals:

    void presetNamesChanged(const QStringList& names);

    void activePreset(int index);

public slots:

    void applyPreset(int index);

private slots:

    void on_presets__activated(int index);
    void on_use__clicked();
    void on_update__clicked();
    void on_delete__clicked();
    void on_revert__clicked();
    void on_apply__clicked();

    void on_alpha1Min__valueChanged(double value);
    void on_alpha1Max__valueChanged(double value);
    void on_betaMin__valueChanged(double value);
    void on_betaMax__valueChanged(double value);
    void on_alpha2Min__valueChanged(double value);
    void on_alpha2Max__valueChanged(double value);
    void on_rangeMin__valueChanged(double value);
    void on_rangeMax__valueChanged(double value);
    void on_syncAlphaMinMax__toggled(bool state);

    void shutdown();
    void updateButtons();
    void addPreset();
    void viewChanged();

private:

    void showViewPreset(const ViewPreset& viewPreset);
    void updateViewLimits();
    ViewPreset getViewPreset(int index) const;
    ViewPreset getActiveViewPreset() const;
    ViewPreset makeViewPreset() const;
    void addViewPreset(const QString& name, const ViewPreset& view);
    void applyViewPreset(const ViewPreset& viewPreset);

    ViewSettings* alphaBetaViewSettings_;
    ViewSettings* alphaRangeViewSettings_;
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

Q_DECLARE_METATYPE(SideCar::GUI::ESScope::ViewPreset)

#endif
