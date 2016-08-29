#include "GUI/DoubleMinMaxValidator.h"
#include "GUI/IntMinMaxValidator.h"

#include "ChannelEditor.h"

using namespace SideCar::GUI::AScope;

ChannelEditor::ChannelEditor(QWidget* parent)
    : QDialog(parent), Ui::ChannelEditor()
{
    setupUi(this);
    new IntMinMaxValidator(this, sampleMin_, sampleMax_);
    new DoubleMinMaxValidator(this, voltageMin_, voltageMax_);
}
