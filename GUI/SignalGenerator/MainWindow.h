#ifndef SIDECAR_GUI_SIGNALGENERATOR_MAINWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_SIGNALGENERATOR_MAINWINDOW_H

#include <inttypes.h>

#include "GUI/MainWindowBase.h"

#include "App.h"

class QModelIndex;

namespace Ui {
class MainWindow;
}

namespace SideCar {
namespace Zeroconf {
class Publisher;
}
namespace GUI {
namespace SignalGenerator {

class Emitter;
class GeneratorConfiguration;
class GeneratorConfigurationsModel;

class MainWindow : public MainWindowBase {
    Q_OBJECT
    using Super = MainWindowBase;

public:
    /** NOTE: the entries here must match the order of the entries in the connectionType_ QComboBox widget.
     */
    enum ConnectionType { kTCP, kMulticast };

    static Logger::Log& Log();

    MainWindow();

    App* getApp() const { return App::GetApp(); }

    size_t getEmitterFrequency() const;

    double getSampleFrequency() const;

    void restoreFromSettings(QSettings& settings);

    void saveToSettings(QSettings& settings);

    void showAndRaise();

signals:

    void sampleFrequencyChanged(double value);

    void doComplexChanged(bool value);

    void reset();

public slots:

    void start();

    void stop();

    void rewind();

private slots:

    void on_addGenerator__clicked();

    void on_removeGenerator__clicked();

    void generatorSelectionCurrentChanged(const QModelIndex& old, const QModelIndex& now);

    void on_sampleCount__valueUpdated(int value);

    void on_radialCount__valueUpdated(int value);

    void on_doComplex__clicked(bool state);

    /** Action handler for the publisher name data entry field. Called when the user leaves the field, or
        pressed ENTER.
    */
    void on_name__editingFinished();

    void on_connectionType__currentIndexChanged(int index);

    /** Action handler for the multicast IP address data entry field. Called when the user leaves the field, or
        pressed ENTER. Saves the new value to the application setttings file, and emits addressChanged() with
        the new value.
    */
    void on_address__editingFinished();

    void on_messageCount__valueUpdated(int value);

    void on_emitterFrequency__valueUpdated(int value);

    void on_emitterFrequencyScale__currentIndexChanged(int value);

    void on_sampleFrequency__valueUpdated(double value);

    void on_sampleFrequencyScale__currentIndexChanged(int value);

    /** Action handler for the Start/Stop push button. Starts or stops playback.
     */
    void on_startStop__clicked();

    /** Action handler for the Rewind push button. Causes all signal generators to reset and start at T0.
     */
    void on_rewind__clicked();

    void writerPublished(const QString& serviceName, const QString& host, uint16_t port);

    void writerSubscriberCountChanged(size_t count);

    void activeConfigurationChanged(GeneratorConfiguration* active);

    void generateMessages();

    void generateOneMessage();

private:
    void publish();

    void addGenerator(GeneratorConfiguration* cfg);

    void closeEvent(QCloseEvent* event);

    struct Private;
    Private* p_;
};

} // namespace SignalGenerator
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
