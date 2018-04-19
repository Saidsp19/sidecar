#include <cmath>

#include "QtCore/QSettings"
#include "QtGui/QValidator"
#include "QtWidgets/QAction"
#include "QtWidgets/QLineEdit"
#include "QtWidgets/QMessageBox"

#include "GUI/DoubleMinMaxValidator.h"

#include "App.h"
#include "Configuration.h"
#include "Settings.h"
#include "SpectrumWidget.h"
#include "ViewEditor.h"

using namespace SideCar::GUI::Spectrum;

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
    ToolWindowBase("ViewEditor", "View Editor", shortcut), Ui::ViewEditor(), spectrumWidget_(0), xMinScaleValue_(1.0),
    xMaxScaleValue_(1.0)
{
    setupUi(this);
    setFixedSize();

    // new DoubleMinMaxValidator(this, yMin_, yMax_);

    App* app = App::GetApp();
    connect(app, SIGNAL(shutdown()), SLOT(shutdown()));

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
ViewEditor::setSpectrumWidget(SpectrumWidget* spectrumWidget)
{
    spectrumWidget_ = spectrumWidget;
    if (spectrumWidget_) updateViewLimits();
}

void
ViewEditor::updateViewLimits()
{
    if (!spectrumWidget_) return;
    showView(spectrumWidget_->getCurrentView());
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
    spectrumWidget_->setCurrentView(makeView());
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
ViewEditor::on_use__clicked()
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
    showView(spectrumWidget_->getCurrentView());
    updateButtons();
}

void
ViewEditor::on_apply__clicked()
{
    spectrumWidget_->setCurrentView(makeView());
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
    if (view.getXMin() != xMinValue_) {
        int scale = int(::log10(std::abs(view.getXMin()))) / 3;
        xMinScale_->setCurrentIndex(scale);
        xMin_->setValue(view.getXMin() / ::pow(10, scale * 3));
    }

    if (view.getXMax() != xMaxValue_) {
        int scale = int(::log10(std::abs(view.getXMax()))) / 3;
        xMaxScale_->setCurrentIndex(scale);
        xMax_->setValue(view.getXMax() / ::pow(10, scale * 3));
    }

    if (view.getYMin() != yMin_->value()) yMin_->setValue(view.getYMin());

    if (view.getYMax() != yMax_->value()) yMax_->setValue(view.getYMax());
}

ViewSettings
ViewEditor::makeView() const
{
    return ViewSettings(xMinValue_, xMaxValue_, yMin_->value(), yMax_->value());
}

void
ViewEditor::updateButtons()
{
    bool hasChanges = false;
    ViewSettings current(makeView());

    if (spectrumWidget_) hasChanges = spectrumWidget_->getCurrentView() != current;

    apply_->setEnabled(hasChanges);
    revert_->setEnabled(hasChanges);

    int presetIndex = presets_->currentIndex();
    bool hasPreset = presetIndex != -1;
    delete_->setEnabled(hasPreset);

    hasChanges = false;
    if (hasPreset) hasChanges = getView(presetIndex) != current;

    use_->setEnabled(hasChanges);
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
ViewEditor::on_xMin__valueChanged(double value)
{
    xMinValue_ = xMin_->value() * xMinScaleValue_;
    updateButtons();
}

void
ViewEditor::on_xMax__valueChanged(double value)
{
    xMaxValue_ = xMax_->value() * xMaxScaleValue_;
    updateButtons();
}

void
ViewEditor::on_yMin__valueChanged(double value)
{
    updateButtons();
}

void
ViewEditor::on_yMax__valueChanged(double value)
{
    updateButtons();
}

void
ViewEditor::on_xMinScale__currentIndexChanged(int index)
{
    xMinScaleValue_ = ::pow(10, index * 3);
    xMinValue_ = xMin_->value() * xMinScaleValue_;
    updateButtons();
}

void
ViewEditor::on_xMaxScale__currentIndexChanged(int index)
{
    xMaxScaleValue_ = ::pow(10, index * 3);
    xMaxValue_ = xMax_->value() * xMaxScaleValue_;
    updateButtons();
}
