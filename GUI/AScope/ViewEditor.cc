#include "QtCore/QSettings"
#include "QtGui/QAction"
#include "QtGui/QLineEdit"
#include "QtGui/QMessageBox"
#include "QtGui/QValidator"

#include "GUI/DoubleMinMaxValidator.h"
#include "GUI/IntMinMaxValidator.h"

#include "App.h"
#include "DisplayView.h"
#include "ViewEditor.h"

using namespace SideCar::GUI::AScope;

static const char* const kViewPresets = "ViewPresets";
static const char* const kName = "Name";
static const char* const kLastActive = "LastActive";

/** An input validator for new preset names. Prohibits the presets QComboBox from accepting a new value unless
    it the validate() method returns Acceptable. This method checks to see if the current input text matches any
    existing names.
*/
class PresetValidator : public QValidator {
public:
    PresetValidator(QComboBox* presets) : QValidator(presets), presets_(presets) {}

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

ViewEditor::ViewEditor(int shortcut) :
    ToolWindowBase("ViewEditor", "View Editor", shortcut), Ui::ViewEditor(), activeVisualizer_(0)
{
    setupUi(this);
    setFixedSize();

    new IntMinMaxValidator(this, gateMin_, gateMax_);
    new IntMinMaxValidator(this, sampleMin_, sampleMax_);
    new DoubleMinMaxValidator(this, rangeMin_, rangeMax_);
    new DoubleMinMaxValidator(this, voltageMin_, voltageMax_);

    connect(getApp(), SIGNAL(distanceUnitsChanged(const QString&)), SLOT(updateDistanceUnits(const QString&)));
    connect(getApp(), SIGNAL(shutdown()), SLOT(shutdown()));

    connect(horizontalUnits_, SIGNAL(currentChanged(int)), SLOT(updateButtons()));
    connect(verticalUnits_, SIGNAL(currentChanged(int)), SLOT(updateButtons()));

    presets_->setValidator(new PresetValidator(presets_));
    connect(presets_->lineEdit(), SIGNAL(editingFinished()), SLOT(addPreset()));

    QSettings settings;
    settings.beginGroup(objectName());

    int count = settings.beginReadArray(kViewPresets);
    for (int index = 0; index < count; ++index) {
        settings.setArrayIndex(index);
        QString name = settings.value(kName).toString();
        addView(name, ViewSettings(settings));
    }
    settings.endArray();

    presets_->setCurrentIndex(settings.value(kLastActive, count - 1).toInt());
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
        getView(index).saveToSettings(settings);
    }
    settings.endArray();

    settings.setValue(kLastActive, presets_->currentIndex());

    settings.endGroup();
}

void
ViewEditor::updateDistanceUnits(const QString& suffix)
{
    rangeMin_->setSuffix(suffix);
    rangeMax_->setSuffix(suffix);
}

void
ViewEditor::activeDisplayViewChanged(DisplayView* displayView)
{
    activeVisualizer_ = displayView ? displayView->getVisualizer() : 0;
    if (activeVisualizer_) { updateViewLimits(); }
}

void
ViewEditor::updateViewLimits()
{
    if (!activeVisualizer_) return;
    showView(activeVisualizer_->getCurrentView());
    updateButtons();
    emit activePreset(update_->isEnabled() ? -1 : presets_->currentIndex());
}

void
ViewEditor::addPreset()
{
    QString name = presets_->lineEdit()->text();
    name = name.trimmed();
    if (name.isEmpty()) return;

    addView(name, makeView());
    presets_->setCurrentIndex(presets_->count() - 1);
    on_presets__activated(presets_->count() - 1);
    QMessageBox::information(this, "New Preset", QString("<p>Created preset '%1'.</p>").arg(name));
    emit presetNamesChanged(getPresetNames());
}

void
ViewEditor::applyPreset(int index)
{
    presets_->setCurrentIndex(index);
    showView(getView(index));
    activeVisualizer_->setCurrentView(makeView());
    updateButtons();
}

QStringList
ViewEditor::getPresetNames() const
{
    QStringList names;
    for (int index = 0; index < presets_->count(); ++index) { names.append(presets_->itemText(index)); }

    return names;
}

void
ViewEditor::on_fetch__clicked()
{
    showView(getView(presets_->currentIndex()));
    updateButtons();
}

void
ViewEditor::on_update__clicked()
{
    if (QMessageBox::question(this, "Update Preset",
                              QString("<p>You are about to update the view preset "
                                      "'%1' with new values. "
                                      "You cannot undo this operation.</p>"
                                      "<p>Are you sure you want to proceed?</p>")
                                  .arg(presets_->currentText()),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        QVariant value;
        value.setValue(makeView());
        presets_->setItemData(presets_->currentIndex(), value);
        updateButtons();
    }
}

void
ViewEditor::on_delete__clicked()
{
    if (QMessageBox::question(this, "Delete Preset",
                              QString("<p>You are about to permanently delete the view preset "
                                      "'%1'. You cannot undo this operation.</p>"
                                      "<p>Are you sure you want to proceed?</p>")
                                  .arg(presets_->currentText()),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        int index = presets_->currentIndex();
        presets_->removeItem(index);
        if (index == presets_->count()) --index;
        presets_->setCurrentIndex(index);
        on_presets__activated(index);
        emit presetNamesChanged(getPresetNames());
    }
}

void
ViewEditor::on_revert__clicked()
{
    showView(activeVisualizer_->getCurrentView());
    updateButtons();
}

void
ViewEditor::on_use__clicked()
{
    showView(getView(presets_->currentIndex()));
    on_apply__clicked();
}

void
ViewEditor::on_apply__clicked()
{
    activeVisualizer_->setCurrentView(makeView());
    updateButtons();
    emit activePreset(update_->isEnabled() ? -1 : presets_->currentIndex());
}

void
ViewEditor::on_presets__activated(int index)
{
    if (index != -1) showView(getView(index));
    updateButtons();
}

void
ViewEditor::showView(const ViewSettings& view)
{
    if (view.getRangeMin() != rangeMin_->value()) rangeMin_->setValue(view.getRangeMin());
    if (view.getRangeMax() != rangeMax_->value()) rangeMax_->setValue(view.getRangeMax());
    if (view.getGateMin() != gateMin_->value()) gateMin_->setValue(view.getGateMin());
    if (view.getGateMax() != gateMax_->value()) gateMax_->setValue(view.getGateMax());
    if (view.getVoltageMin() != voltageMin_->value()) voltageMin_->setValue(view.getVoltageMin());
    if (view.getVoltageMax() != voltageMax_->value()) voltageMax_->setValue(view.getVoltageMax());
    if (view.getSampleMin() != sampleMin_->value()) sampleMin_->setValue(view.getSampleMin());
    if (view.getSampleMax() != sampleMax_->value()) sampleMax_->setValue(view.getSampleMax());
    if (view.isShowingRanges() != (horizontalUnits_->currentIndex() == 0))
        horizontalUnits_->setCurrentIndex(view.isShowingRanges() ? 0 : 1);
    if (view.isShowingVoltages() != (verticalUnits_->currentIndex() == 0))
        verticalUnits_->setCurrentIndex(view.isShowingVoltages() ? 0 : 1);
}

ViewSettings
ViewEditor::makeView() const
{
    bool showingRanges = horizontalUnits_->currentIndex() == 0;
    bool showingVoltages = verticalUnits_->currentIndex() == 0;
    return ViewSettings(rangeMin_->value(), rangeMax_->value(), gateMin_->value(), gateMax_->value(),
                        voltageMin_->value(), voltageMax_->value(), sampleMin_->value(), sampleMax_->value(),
                        showingRanges, showingVoltages);
}

void
ViewEditor::updateButtons()
{
    bool hasChanges = false;
    ViewSettings current(makeView());

    if (activeVisualizer_) hasChanges = activeVisualizer_->getCurrentView() != current;

    apply_->setEnabled(hasChanges);
    revert_->setEnabled(hasChanges);
    use_->setEnabled(hasChanges);

    int presetIndex = presets_->currentIndex();
    bool hasPreset = presetIndex != -1;
    delete_->setEnabled(hasPreset);

    hasChanges = false;
    if (hasPreset) hasChanges = getView(presetIndex) != current;

    fetch_->setEnabled(hasChanges);
    update_->setEnabled(hasChanges);
}

ViewSettings
ViewEditor::getView(int index) const
{
    return presets_->itemData(index).value<ViewSettings>();
}

void
ViewEditor::addView(const QString& name, const ViewSettings& view)
{
    QVariant value;
    value.setValue(view);
    presets_->addItem(name, value);
}

void
ViewEditor::on_rangeMin__valueChanged(double value)
{
    if (value > rangeMax_->value()) rangeMax_->setValue(value + 1);
    updateButtons();
}

void
ViewEditor::on_rangeMax__valueChanged(double value)
{
    if (value < rangeMin_->value()) rangeMin_->setValue(value - 1);
    updateButtons();
}

void
ViewEditor::on_gateMin__valueChanged(int value)
{
    if (value > gateMax_->value()) gateMax_->setValue(value + 1);
    updateButtons();
}

void
ViewEditor::on_gateMax__valueChanged(int value)
{
    if (value < gateMin_->value()) gateMin_->setValue(value - 1);
    updateButtons();
}

void
ViewEditor::on_voltageMin__valueChanged(double value)
{
    if (value > voltageMax_->value()) voltageMax_->setValue(value + 1);
    updateButtons();
}

void
ViewEditor::on_voltageMax__valueChanged(double value)
{
    if (value < voltageMin_->value()) voltageMin_->setValue(value - 1);
    updateButtons();
}

void
ViewEditor::on_sampleMin__valueChanged(int value)
{
    if (value > sampleMax_->value()) sampleMax_->setValue(value + 1);
    updateButtons();
}

void
ViewEditor::on_sampleMax__valueChanged(int value)
{
    if (value < sampleMin_->value()) sampleMin_->setValue(value - 1);
    updateButtons();
}
