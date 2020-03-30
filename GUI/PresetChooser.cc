#include "QtCore/QEvent"
#include "QtWidgets/QBoxLayout"
#include "QtGui/QFont"
#include "QtGui/QMouseEvent"
#include "QtGui/QPalette"

#include "Logger/Log.h"
#include "PresetChooser.h"
#include "PresetManager.h"
#include "Utils.h"

using namespace SideCar::GUI;

Logger::Log&
PresetChooser::Log()
{
    Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.PresetChooser");
    return log_;
}

PresetChooser::PresetChooser(PresetManager* presetManager, QWidget* parent) :
    Super(parent), presetManager_(presetManager)
{
    setFocusPolicy(Qt::NoFocus);
    setSizeAdjustPolicy(QComboBox::AdjustToContents);
    addItems(presetManager->getPresetNames());
    setCurrentIndex(presetManager->getActivePresetIndex());
    setToolTip("Select active preset. CTRL+Click to save changes.");

    setDirtyState(presetManager->getActivePresetIndex(), presetManager->getActiveIsDirty());

    connect(this, SIGNAL(currentIndexChanged(int)), presetManager, SLOT(applyPreset(int)));
    connect(presetManager, SIGNAL(activePresetChanged(int)), SLOT(activePresetChanged(int)));
    connect(presetManager, SIGNAL(presetNamesChanged(const QStringList&)),
            SLOT(presetNamesChanged(const QStringList&)));
    connect(presetManager, SIGNAL(presetDirtyStateChanged(int, bool)), SLOT(setDirtyState(int, bool)));

    installEventFilter(this);
}

void
PresetChooser::activePresetChanged(int index)
{
    blockSignals(true);
    setCurrentIndex(index);
    blockSignals(false);
    setDirtyState(index, presetManager_->getPresetIsDirty(index));
}

void
PresetChooser::presetNamesChanged(const QStringList& names)
{
    int index = currentIndex();
    blockSignals(true);
    clear();
    addItems(names);
    setCurrentIndex(index);
    blockSignals(false);
}

void
PresetChooser::setDirtyState(int index, bool isDirty)
{
    static Logger::ProcLog log("setDirtyState", Log());
    LOGINFO << "index: " << index << " isDirty: " << isDirty << std::endl;
    QPalette p(palette());
    p.setColor(QPalette::Text, isDirty ? WarningRedColor() : NormalTextColor(QPalette::ButtonText));
    setPalette(p);
    update();
}

void
PresetChooser::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && event->modifiers() == Qt::ControlModifier) {
        presetManager_->savePreset(currentIndex());
        event->accept();
        return;
    }

    Super::mousePressEvent(event);
}
