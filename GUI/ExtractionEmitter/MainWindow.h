#ifndef SIDECAR_GUI_MAINWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_MAINWINDOW_H

#include "GUI/MainWindowBase.h"
#include "Messages/Extraction.h"

#include "App.h"
#include "ui_MainWindow.h"

namespace SideCar {
namespace Zeroconf { class Publisher; }
namespace GUI {

class MessageWriter;

namespace ExtractionEmitter {

class ViewModel;
class ViewModelChanger;

class MainWindow : public MainWindowBase, private Ui::MainWindow
{
    Q_OBJECT
    using Super = MainWindowBase;
public:

    /** NOTE: the entries here must match the order of the entries in the connectionType_ QComboBox widget.
     */
    enum ConnectionType {
	kTCP,
	kMulticast
    };

    static Logger::Log& Log();

    MainWindow();

    App* getApp() const { return App::GetApp(); }

private slots:

    void on_editName__clicked();

    void on_connectionType__currentIndexChanged(int index);

    void on_hostName__editingFinished();

    void on_add__clicked();

    void on_clear__clicked();

    void on_send__clicked();

    void on_range__valueChanged(int value);

    void on_azimuth__valueChanged(int value);

    void pendingCurrentChanged(const QModelIndex&, const QModelIndex&);

    void writerPublished(const QString& serviceName, const QString& host,
                         uint16_t port);

    void writerSubscriberCountChanged(size_t count);

private:

    void makeWriter();

    void removeWriter();

    void updateWindowActions();

    MessageWriter* writer_;
    ViewModel* model_;
    ViewModelChanger* entryChanger_;
    QString lastHostName_;
};

} // end namespace ExtractionEmitter
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
