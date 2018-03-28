#ifndef SIDECAR_GUI_PLAYBACK_BROWSERWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_PLAYBACK_BROWSERWINDOW_H

#include "GUI/ToolWindowBase.h"
#include "ui_BrowserWindow.h"

namespace SideCar {
namespace GUI {
namespace Playback {

class RecordingInfo;
class RecordingModel;

class BrowserWindow : public ToolWindowBase, public Ui::BrowserWindow {
    Q_OBJECT
    using Super = ToolWindowBase;

public:
    static Logger::Log& Log();

    BrowserWindow(int shortcut);

    bool browse(const QString& directory);

signals:

    void loadRequest(const QString& path);

private slots:

    void handleDoubleClick(const QModelIndex& index);
    void on_browse__clicked();
    void on_browseDir__activated(int index);
    void updateColumns(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    void currentSelectionChanged(const QModelIndex& current, const QModelIndex& previous);

private:
    bool eventFilter(QObject* object, QEvent* event);

    void adjustColumnSizes();

    void resizeEvent(QResizeEvent* event);

    void loadSelected();

    RecordingModel* model_;
};

} // end namespace Playback
} // end namespace GUI
} // end namespace SideCar

#endif
