#include "QtCore/QSettings"
#include "QtGui/QAction"
#include "QtGui/QLineEdit"
#include "QtGui/QMessageBox"
#include "QtGui/QValidator"

#include "App.h"
#include "ViewEditor.h"
#include "ViewSettings.h"

using namespace SideCar::GUI::ESScope;

static const char* const kViewPresets = "ViewPresets";
static const char* const kName = "Name";
static const char* const kLastActive = "LastActive";
static const char* const kSyncAlphaMinMax = "SyncAlphaMinMax";

static const char* const kAlpha1Min = "Alpha1Min";
static const char* const kAlpha1Max = "Alpha1Max";
static const char* const kBetaMin = "BetaMin";
static const char* const kBetaMax = "BetaMax";
static const char* const kAlpha2Min = "Alpha2Min";
static const char* const kAlpha2Max = "Alpha2Max";
static const char* const kRangeMin = "RangeMin";
static const char* const kRangeMax = "RangeMax";

int const ViewPreset::kMetaTypeId =
    qRegisterMetaType<ViewPreset>("ViewPreset");

ViewPreset::ViewPreset(QSettings& settings)
{
    restoreFromSettings(settings);
}

void
ViewPreset::restoreFromSettings(QSettings& settings)
{
    alpha1Min = settings.value(kAlpha1Min, alpha1Min).toDouble();
    alpha1Max = settings.value(kAlpha1Max, alpha1Max).toDouble();
    betaMin = settings.value(kBetaMin, betaMin).toDouble();
    betaMax = settings.value(kBetaMax, betaMax).toDouble();
    alpha2Min = settings.value(kAlpha2Min, alpha2Min).toDouble();
    alpha2Max = settings.value(kAlpha2Max, alpha2Max).toDouble();
    rangeMin = settings.value(kRangeMin, rangeMin).toDouble();
    rangeMax = settings.value(kRangeMax, rangeMax).toDouble();
}

void
ViewPreset::saveToSettings(QSettings& settings) const
{
    settings.setValue(kAlpha1Min, alpha1Min);
    settings.setValue(kAlpha1Max, alpha1Max);
    settings.setValue(kBetaMin, betaMin);
    settings.setValue(kBetaMax, betaMax);
    settings.setValue(kAlpha2Min, alpha2Min);
    settings.setValue(kAlpha2Max, alpha2Max);
    settings.setValue(kRangeMin, rangeMin);
    settings.setValue(kRangeMax, rangeMax);
}

/** An input validator for new preset names. Prohibits the presets QComboBox from accepting a new value unless
    it the validate() method returns Acceptable. This method checks to see if the current input text matches any
    existing names.
*/
class PresetValidator : public QValidator
{
public:
    PresetValidator(QComboBox* presets)
	: QValidator(presets), presets_(presets) {}

    State validate(QString& input, int& pos) const
	{
	    QString trimmed = input.trimmed();
	    input = trimmed;
	    if (trimmed.isEmpty()) return Invalid;
	    if (presets_->findText(trimmed) != -1) return Intermediate;
	    return Acceptable;
	}

private:
    QComboBox* presets_;
};

ViewEditor::ViewEditor(int shortcut)
    : ToolWindowBase("ViewEditor", "View Editor", shortcut), Ui::ViewEditor(),
      alphaBetaViewSettings_(0), alphaRangeViewSettings_(0)
{
    setupUi(this);
    setFixedSize();

    App* app = App::GetApp();
    connect(app, SIGNAL(shutdown()), SLOT(shutdown()));

    presets_->setValidator(new PresetValidator(presets_));
    connect(presets_->lineEdit(), SIGNAL(editingFinished()),
            SLOT(addPreset()));

    QSettings settings;
    syncAlphaMinMax_->setChecked(
	settings.value(kSyncAlphaMinMax, false).toBool());
    on_syncAlphaMinMax__toggled(syncAlphaMinMax_->isChecked());

    settings.beginGroup(objectName());
    int count = settings.beginReadArray(kViewPresets);
    for (int index = 0; index < count; ++index) {
	settings.setArrayIndex(index);
	QString name = settings.value(kName).toString();
	addViewPreset(name, ViewPreset(settings));
    }
    settings.endArray();
    presets_->setCurrentIndex(
	settings.value(kLastActive, count - 1).toInt());
    on_presets__activated(count - 1);
    settings.endGroup();

    emit presetNamesChanged(getPresetNames());
}

void
ViewEditor::shutdown()
{
    QSettings settings;
    settings.beginGroup(objectName());
    settings.beginWriteArray(kViewPresets, presets_->count());
    for (int index = 0; index < presets_->count(); ++index) {
	settings.setArrayIndex(index);
	settings.setValue(kName, presets_->itemText(index));
	getViewPreset(index).saveToSettings(settings);
    }
    settings.endArray();
    settings.setValue(kLastActive, presets_->currentIndex());
    settings.endGroup();
}

void
ViewEditor::setViewSettings(ViewSettings* alphaBetaViewSettings,
                            ViewSettings* alphaRangeViewSettings)
{
    alphaBetaViewSettings_ = alphaBetaViewSettings;
    connect(alphaBetaViewSettings_, SIGNAL(viewChanged()),
            SLOT(viewChanged()));
    alphaRangeViewSettings_ = alphaRangeViewSettings;
    connect(alphaRangeViewSettings_, SIGNAL(viewChanged()),
            SLOT(viewChanged()));
    if (presets_->currentIndex() != -1)
	applyPreset(presets_->currentIndex());
}

void
ViewEditor::updateViewLimits()
{
    if (! alphaBetaViewSettings_ || ! alphaRangeViewSettings_) return;
    showViewPreset(getActiveViewPreset());
    updateButtons();
}

void
ViewEditor::addPreset()
{
    QString name = presets_->lineEdit()->text();
    name = name.trimmed();
    if (name.isEmpty()) return;

    addViewPreset(name, makeViewPreset());
    presets_->setCurrentIndex(presets_->count() - 1);
    on_presets__activated(presets_->count() - 1);
    QMessageBox::information(
	this, "New Preset",
	QString("<p>Created preset '%1'.</p>").arg(name));
    emit presetNamesChanged(getPresetNames());
}

void
ViewEditor::applyPreset(int index)
{
    presets_->setCurrentIndex(index);
    ViewPreset viewPreset(getViewPreset(index));
    showViewPreset(viewPreset);
    applyViewPreset(viewPreset);
}

void
ViewEditor::applyViewPreset(const ViewPreset& viewPreset)
{
    alphaBetaViewSettings_->setViewBounds(
	ViewBounds(viewPreset.alpha1Min, viewPreset.alpha1Max,
                   viewPreset.betaMin, viewPreset.betaMax));
    alphaRangeViewSettings_->setViewBounds(
	ViewBounds(viewPreset.alpha2Min, viewPreset.alpha2Max,
                   viewPreset.rangeMin, viewPreset.rangeMax));
    updateButtons();
}

QStringList
ViewEditor::getPresetNames() const
{
    QStringList names;
    for (int index = 0; index < presets_->count(); ++index) {
	names.append(presets_->itemText(index));
    }

    return names;
}

void
ViewEditor::on_use__clicked()
{
    showViewPreset(getViewPreset(presets_->currentIndex()));
    updateButtons();
}

void
ViewEditor::on_update__clicked()
{
    if (QMessageBox::question(
            this, "Update Preset",
            QString("<p>You are about to update the view preset "
                    "'%1' with new values. "
                    "You cannot undo this operation.</p>"
                    "<p>Are you sure you want to proceed?</p>")
            .arg(presets_->currentText()),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
	QVariant value;
	value.setValue(makeViewPreset());
	presets_->setItemData(presets_->currentIndex(), value);
	updateButtons();
    }
}

void
ViewEditor::on_delete__clicked()
{
    if (QMessageBox::question(
            this, "Delete Preset",
            QString("<p>You are about to permanently delete the view preset "
                    "'%1'. You cannot undo this operation.</p>"
                    "<p>Are you sure you want to proceed?</p>")
            .arg(presets_->currentText()),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
	int index = presets_->currentIndex();
	presets_->removeItem(index);
	if (index == presets_->count())
	    --index;
	presets_->setCurrentIndex(index);
	on_presets__activated(index);
	emit presetNamesChanged(getPresetNames());
    }
}

void
ViewEditor::on_revert__clicked()
{
    showViewPreset(getActiveViewPreset());
    updateButtons();
}

void
ViewEditor::on_apply__clicked()
{
    applyViewPreset(makeViewPreset());
}

void
ViewEditor::on_presets__activated(int index)
{
    if (index != -1)
	showViewPreset(getViewPreset(index));
    updateButtons();
}

void
ViewEditor::showViewPreset(const ViewPreset& viewPreset)
{
    alpha1Min_->setValue(viewPreset.alpha1Min);
    alpha1Max_->setValue(viewPreset.alpha1Max);
    betaMin_->setValue(viewPreset.betaMin);
    betaMax_->setValue(viewPreset.betaMax);
    alpha2Min_->setValue(viewPreset.alpha2Min);
    alpha2Max_->setValue(viewPreset.alpha2Max);
    rangeMin_->setValue(viewPreset.rangeMin);
    rangeMax_->setValue(viewPreset.rangeMax);
}

ViewPreset
ViewEditor::makeViewPreset() const
{
    return ViewPreset(alpha1Min_->value(), alpha1Max_->value(),
                      betaMin_->value(), betaMax_->value(),
                      alpha2Min_->value(), alpha2Max_->value(),
                      rangeMin_->value(), rangeMax_->value());
}

ViewPreset
ViewEditor::getActiveViewPreset() const
{
    return ViewPreset(alphaBetaViewSettings_->getXMin(),
                      alphaBetaViewSettings_->getXMax(),
                      alphaBetaViewSettings_->getYMin(),
                      alphaBetaViewSettings_->getYMax(),
                      alphaRangeViewSettings_->getXMin(),
                      alphaRangeViewSettings_->getXMax(),
                      alphaRangeViewSettings_->getYMin(),
                      alphaRangeViewSettings_->getYMax());
}

void
ViewEditor::updateButtons()
{
    bool hasChanges = false;
    ViewPreset current(makeViewPreset());

    if (alphaBetaViewSettings_ && alphaRangeViewSettings_) {
	ViewPreset active(getActiveViewPreset());
	hasChanges = active != current;
    }

    apply_->setEnabled(hasChanges);
    revert_->setEnabled(hasChanges);

    int presetIndex = presets_->currentIndex();
    bool hasPreset = presetIndex != -1;
    delete_->setEnabled(hasPreset);

    hasChanges = false;
    if (hasPreset)
	hasChanges = getViewPreset(presetIndex) != current;

    use_->setEnabled(hasChanges);
    update_->setEnabled(hasChanges);

    emit activePreset(update_->isEnabled() ? -1 : presets_->currentIndex());
}

ViewPreset
ViewEditor::getViewPreset(int index) const
{
    return presets_->itemData(index).value<ViewPreset>();
}

void
ViewEditor::addViewPreset(const QString& name, const ViewPreset& viewPreset)
{
    QVariant value;
    value.setValue(viewPreset);
    presets_->addItem(name, value);
}

void
ViewEditor::on_alpha1Min__valueChanged(double value)
{
    if (syncAlphaMinMax_->isChecked())
	alpha2Min_->setValue(value);
    updateButtons();
}

void
ViewEditor::on_alpha1Max__valueChanged(double value)
{
    if (syncAlphaMinMax_->isChecked())
	alpha2Max_->setValue(value);
    updateButtons();
}

void
ViewEditor::on_betaMin__valueChanged(double value)
{
    updateButtons();
}

void
ViewEditor::on_betaMax__valueChanged(double value)
{
    updateButtons();
}

void
ViewEditor::on_alpha2Min__valueChanged(double value)
{
    updateButtons();
}

void
ViewEditor::on_alpha2Max__valueChanged(double value)
{
    updateButtons();
}

void
ViewEditor::on_rangeMin__valueChanged(double value)
{
    updateButtons();
}

void
ViewEditor::on_rangeMax__valueChanged(double value)
{
    updateButtons();
}

void
ViewEditor::on_syncAlphaMinMax__toggled(bool state)
{
    alpha2Min_->setEnabled(! state);
    alpha2Max_->setEnabled(! state);
    if (state) {
	alpha2Min_->setValue(alpha1Min_->value());
	alpha2Max_->setValue(alpha1Max_->value());
	updateButtons();
    }

    QSettings settings;
    settings.setValue(kSyncAlphaMinMax, state);
}

void
ViewEditor::viewChanged()
{
    updateViewLimits();
    updateButtons();
}
