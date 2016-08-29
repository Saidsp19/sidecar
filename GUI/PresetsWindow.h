#ifndef SIDECAR_GUI_PRESETSWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_PRESETSWINDOW_H

#include "GUI/ToolWindowBase.h"

class QListWidgetItem;

namespace Logger { class Log; }

namespace Ui { class PresetsWindow; }

namespace SideCar {
namespace GUI {

class PresetManager;

/** Floating tool window that shows the message time for the data shown in a PPIWidget, and the range/azimuth
    values for the cursor. The message values may be current or historical.
*/
class PresetsWindow : public ToolWindowBase
{
    Q_OBJECT
    using Super = ToolWindowBase;
public:
    
    /** Obtain the Log device to use for PresetsWindow objects

        \return Log reference
    */
    static Logger::Log& Log();
    
    /** Constructor. Creates and initializes window widgets.

        \param action the QAction object that controls window visibility

	\param presetManager the PresetManager that manages the collection of
        active Preset objects.
    */
    PresetsWindow(int shortcut, PresetManager* presetManager);

private slots:
    
    /** Notification handler invoked when the 'New' button is clicked.
     */
    void newPreset();
    
    /** Notification handler invoked when the 'Delete' button is clicked.
     */
    void deletePreset();

    /** Notification handler invoked when the 'Activate' button is clicked.
     */
    void activatePreset();

    void savePreset();

    void revertPreset();

    /** Notification handler invoked when a preset name is double-clicked. Activates the proper preset if not
        already active.

        \param item the object that received the double-click event
    */
    void presetDoubleClicked(QListWidgetItem* item);

    void updateButtons();

    void presetDirtyStateChanged(int index, bool isDirty);

    void activePresetChanged(int index);

    void presetNamesChanged(const QStringList& names);

    void selectionChanged();

private:

    void updateActive();

    QString getSelectedName() const;

    QString getActiveName() const;

    int getSelectedRow() const;

    void showEvent(QShowEvent* event);

    PresetManager* presetManager_;
    Ui::PresetsWindow* gui_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
