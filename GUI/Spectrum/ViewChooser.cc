#include "QtWidgets/QComboBox"
#include "QtWidgets/QHBoxLayout"

#include "ViewChooser.h"
#include "ViewEditor.h"

using namespace SideCar::GUI::Spectrum;

ViewChooser::ViewChooser(ViewEditor* viewEditor, QWidget* parent) : Super(parent)
{
    QBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(6);

    chooser_ = new QComboBox(this);
    chooser_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    chooser_->addItems(viewEditor->getPresetNames());
    chooser_->setCurrentIndex(viewEditor->getActivePresetIndex());
    chooser_->setFocusPolicy(Qt::NoFocus);

    layout->addWidget(chooser_);
    layout->addSpacing(6);
    layout->addStretch(1);

    connect(chooser_, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), viewEditor,
            &ViewEditor::applyPreset);
    connect(viewEditor, &ViewEditor::activePreset, chooser_, &QComboBox::setCurrentIndex);
    connect(viewEditor, &ViewEditor::presetNamesChanged, this, &ViewChooser::presetNamesChanged);
}

void
ViewChooser::presetNamesChanged(const QStringList& names)
{
    int index = chooser_->currentIndex();
    chooser_->blockSignals(true);
    chooser_->clear();
    chooser_->addItems(names);
    chooser_->setCurrentIndex(index);
    chooser_->blockSignals(false);
}
