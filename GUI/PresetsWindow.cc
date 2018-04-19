#include "QtWidgets/QInputDialog"
#include "QtWidgets/QMessageBox"

#include "LogUtils.h"
#include "PresetManager.h"
#include "PresetsWindow.h"

#include "ui_PresetsWindow.h"

using namespace SideCar;
using namespace SideCar::GUI;

static const char* const kDirtySuffix = " (unsaved)";

Logger::Log&
PresetsWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.PresetsWindow");
    return log_;
}

PresetsWindow::PresetsWindow(int shortcut, PresetManager* presetManager) :
    Super("PresetsWindow", "Presets", shortcut), presetManager_(presetManager), gui_(new Ui::PresetsWindow)
{
    gui_->setupUi(this);
    connect(gui_->new_, SIGNAL(clicked()), SLOT(newPreset()));
    connect(gui_->delete_, SIGNAL(clicked()), SLOT(deletePreset()));
    connect(gui_->activate_, SIGNAL(clicked()), SLOT(activatePreset()));
    connect(gui_->save_, SIGNAL(clicked()), SLOT(savePreset()));
    connect(gui_->revert_, SIGNAL(clicked()), SLOT(revertPreset()));
    connect(gui_->name_, SIGNAL(textEdited(const QString&)), SLOT(updateButtons()));
    connect(gui_->name_, SIGNAL(returnPressed()), SLOT(newPreset()));
    connect(gui_->presets_, SIGNAL(itemDoubleClicked(QListWidgetItem*)), SLOT(presetDoubleClicked(QListWidgetItem*)));
    connect(gui_->presets_, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), SLOT(selectionChanged()));
    connect(presetManager, SIGNAL(presetDirtyStateChanged(int, bool)), SLOT(presetDirtyStateChanged(int, bool)));
    connect(presetManager, SIGNAL(activePresetChanged(int)), SLOT(activePresetChanged(int)));
    connect(presetManager, SIGNAL(presetNamesChanged(const QStringList&)),
            SLOT(presetNamesChanged(const QStringList&)));

    gui_->presets_->addItems(presetManager_->getPresetNames());
    updateActive();
    updateButtons();
}

void
PresetsWindow::updateButtons()
{
    int selected = getSelectedRow();
    int active = presetManager_->getActivePresetIndex();
    QString name = gui_->name_->text().simplified();
    bool uniqueName = !name.isEmpty() && !presetManager_->getPreset(name);
    bool hasSelection = selected != -1;
    bool activeSelected = hasSelection && selected == active;
    bool notFirst = hasSelection && selected != 0;
    bool isDirty = hasSelection && presetManager_->getPresetIsDirty(selected);

    gui_->new_->setEnabled(uniqueName);
    gui_->delete_->setEnabled(hasSelection && notFirst);
    gui_->activate_->setEnabled(hasSelection && !activeSelected);
    gui_->save_->setEnabled(isDirty);
    gui_->revert_->setEnabled(isDirty);
}

void
PresetsWindow::selectionChanged()
{
    updateButtons();
}

void
PresetsWindow::newPreset()
{
    QString name = gui_->name_->text().simplified();
    if (name.isEmpty()) return;

    if (presetManager_->addPreset(name)) {
        gui_->presets_->setCurrentRow(gui_->presets_->count() - 1);
        updateButtons();
        gui_->name_->clear();
    }
}

void
PresetsWindow::deletePreset()
{
    if (QMessageBox::question(this, "Delete Preset",
                              QString("<p>You are about to permanently delete the "
                                      "preset '%1'. You cannot undo this "
                                      "action.</p>"
                                      "<p>Are you sure you want to proceed?</p>")
                                  .arg(getSelectedName()),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        if (presetManager_->removePreset(gui_->presets_->currentRow())) { updateButtons(); }
    }
}

void
PresetsWindow::activatePreset()
{
    Logger::ProcLog log("activatePreset", Log());
    LOGINFO << "row: " << gui_->presets_->currentRow() << std::endl;
    if (presetManager_->applyPreset(gui_->presets_->currentRow())) { updateButtons(); }
}

void
PresetsWindow::savePreset()
{
    presetManager_->savePreset(getSelectedRow());
    updateButtons();
}

void
PresetsWindow::revertPreset()
{
    if (QMessageBox::question(this, "Revert Preset",
                              QString("<p>You are about to undo all changes made to '%1' "
                                      "preset, reverting to the last saved configuration.</p>"
                                      "<p>Are you sure you want to proceed?</p>")
                                  .arg(getSelectedName()),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        presetManager_->restorePreset(getSelectedRow());
        updateButtons();
    }
}

void
PresetsWindow::presetDoubleClicked(QListWidgetItem* item)
{
    if (getSelectedRow() != presetManager_->getActivePresetIndex()) activatePreset();
}

void
PresetsWindow::presetDirtyStateChanged(int index, bool isDirty)
{
    Logger::ProcLog log("presetDirtyStateChanged", Log());
    LOGINFO << "index: " << index << " isDirty: " << isDirty << std::endl;
    QString name = presetManager_->getPresetName(index);
    if (isDirty) name += kDirtySuffix;
    gui_->presets_->item(index)->setText(name);
    updateButtons();
}

void
PresetsWindow::activePresetChanged(int index)
{
    Logger::ProcLog log("activePresetChanged", Log());
    LOGINFO << "index: " << index << std::endl;
    updateActive();
    updateButtons();
}

void
PresetsWindow::presetNamesChanged(const QStringList& names)
{
    Logger::ProcLog log("presetNamesChanged", Log());
    LOGINFO << std::endl;
    int alive = gui_->presets_->count();
    for (int index = 0; index < names.size(); ++index) {
        QString name = names[index];
        LOGDEBUG << "name: " << name << std::endl;
        if (presetManager_->getPresetIsDirty(index)) name += kDirtySuffix;
        if (index < alive)
            gui_->presets_->item(index)->setText(name);
        else {
            new QListWidgetItem(name, gui_->presets_);
        }
    }

    while (alive > names.size()) delete gui_->presets_->takeItem(--alive);

    updateButtons();
}

void
PresetsWindow::updateActive()
{
    int active = presetManager_->getActivePresetIndex();
    for (int index = 0; index < gui_->presets_->count(); ++index) {
        QListWidgetItem* item = gui_->presets_->item(index);
        QFont font = item->font();
        font.setBold(active == index);
        item->setFont(font);
    }
}

QString
PresetsWindow::getSelectedName() const
{
    return gui_->presets_->item(getSelectedRow())->text();
}

QString
PresetsWindow::getActiveName() const
{
    return gui_->presets_->item(presetManager_->getActivePresetIndex())->text();
}

int
PresetsWindow::getSelectedRow() const
{
    return gui_->presets_->currentRow();
}

void ::PresetsWindow::showEvent(QShowEvent* event)
{
    Super::showEvent(event);
    gui_->name_->setFocus(Qt::ActiveWindowFocusReason);
}
