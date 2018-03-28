#ifndef SIDECAR_GUI_ASCOPE_HISTORYPOSITION_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_HISTORYPOSITION_H

#include "QtCore/QObject"
#include "QtCore/QSet"
#include "QtCore/QStringList"

#include "HistoryFrame.h"

class QSettings;

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace AScope {

class ChannelConnection;
class AzimuthLatch;
class History;
class Visualizer;

/** History position for a given Visualizer object. All Visualizer plot widgets share the same History object,
    but have their own HistoryPosition object so that they may show different views of the historical data. Each
    view may show historical or live data independent of the other views; the viewingPast() method controls
    which data is shown.
*/
class HistoryPosition : public QObject {
    Q_OBJECT
public:
    static Logger::Log& Log();

    /** Constructor. The initial position is always set to show current data.
     */
    HistoryPosition(Visualizer* parent, AzimuthLatch* azimuthLatch);

    void restoreFromSettings(QSettings& settings);

    void saveToSettings(QSettings& settings);

    bool isViewingPast() const { return viewingPast_; }

    bool isSynchronized() const { return synchronized_; }

    bool isInfoReporter() const { return infoReporter_; }

    /** Obtain the current view position.

        \return current view position
    */
    int getPastPosition() const { return position_; }

    Messages::PRIMessage::Ref getMessage(int slot) const;

signals:

    /** Notification sent out when the position changes in such a way that the view has changed. This is
        primarily to cause display views to repaint.
    */
    void viewChanged();

    void viewingPastChanged(bool state);

    void infoUpdate(const Messages::PRIMessage::Ref& msg);

public slots:

    /** Change the viewing position.

        \param position new viewing position
    */
    void setPosition(int position);

    void viewingPast(bool state);

    void setSynchronized(bool state);

    void setInfoReporter(bool state);

private slots:

    void liveFrameChanged();

    void pastFrozen();

    void pastThawed();

    void azimuthLatchChanged(bool enabled, double azimuth, bool relatch, const QString& active);

    void channelNamesChanged(const QSet<QString>& names);

private:
    bool checkAzimuthLatch();

    Visualizer* parent_;
    AzimuthLatch* azimuthLatch_;
    History& history_;
    HistoryFrame pastFrame_;
    int position_;
    bool viewingPast_;
    bool synchronized_;
    bool infoReporter_;

    QStringList names_;
    ChannelConnection* latchChannel_;
    double lastAzimuth_;
    double latch_;

    bool azLatchEnabled_;
    QString azLatchChannel_;
    double azLatchAzimuth_;
    bool azLatchRelatch_;
    bool caught_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
