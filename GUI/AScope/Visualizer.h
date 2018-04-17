#ifndef SIDECAR_GUI_ASCOPE_VISUALIZER_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_VISUALIZER_H

#include <vector>

#include "QtCore/QBasicTimer"
#include "QtCore/QRectF"
#include "QtCore/QSet"
#include "QtCore/QTime"

#include "QtGui/QMatrix"
#include "QtGui/QPixmap"
#include "QtWidgets/QWidget"

#include "IO/Printable.h"

#include "AzimuthLatch.h"
#include "ChannelConnection.h"
#include "ViewSettings.h"

class QSettings;

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace AScope {

class AzimuthLatch;
class DisplayView;
class HistoryPosition;
class ViewChanger;

/** Render widget for the AScope application. Draws data from one or more VideoChannel connections. Maintains a
    stack of ViewSettings settings which define the current view. Panning and zooming add to the stack, while
    menu actions exist to pop entries off, returning to a previous view.
*/
class Visualizer : public QWidget {
    Q_OBJECT
    using Super = QWidget;

public:
    static Logger::Log& Log();

    /** Constructor. Creates a new Visualizer from scratch.

        \param parent widget parent
    */
    Visualizer(DisplayView* parent, AzimuthLatch* azimuthLatch);

    ~Visualizer();

    bool isFrozen() const;

    bool isShowingGrid() const { return showGrid_; }

    bool isShowingPeakBars() const { return showPeakBars_; }

    HistoryPosition* getHistoryPosition() const { return historyPosition_; }

    void saveToSettings(QSettings& settings);

    void restoreFromSettings(QSettings& settings);

    void duplicate(const Visualizer* other);

    void addVideoChannel(VideoChannel& channel, bool visible, bool showPeakBars);

    VideoChannel* removeChannelConnection(int index);

    const QSet<QString>& getConnectionNames() const { return connectionNames_; }

    ChannelConnection* getChannelConnection(const QString& name) const;

    /** Obtain the ChannelConnection object at a particular index.

        \return ChannelConnection object
    */
    ChannelConnection* getChannelConnection(int row) const { return connections_.at(row); }

    int findVideoChannel(VideoChannel* channel) const;

    bool isConnectedToChannel(VideoChannel* channel) const { return findVideoChannel(channel) != -1; }

    /** Obtain the number of held ChannelConnection objects

        \return ChannelConnection counnt
    */
    int getNumChannelConnections() const { return connections_.size(); }

    const ViewSettings& getFullView() const { return viewStack_.front(); }

    const ViewSettings& getCurrentView() const { return viewStack_.back(); }

    void setShowingRanges(bool state);

    void setShowingVoltages(bool state);

    void setCurrentView(const ViewSettings& viewSettings);

    void dupView();

    bool canPopView() const { return viewStack_.size() > 1; }

    const QMatrix& getTransform() const { return transform_; }

    const QMatrix& getInverseTransform() const { return inverseTransform_; }

    QPointF fromViewToWorld(const QPoint& point) { return inverseTransform_.map(QPointF(point)); }

signals:

    void transformChanged();

    void pointerMoved(const QPoint& local, const QPointF& world);

    void channelConnectionsChanged(const QSet<QString>& names);

public slots:

    void centerAtCursor();

    void setBackground(const QImage& background);

    void needUpdate() { needUpdate_ = true; }

    void setFrozen(bool state);

    void setShowGrid(bool state);

    void setShowPeakBars(bool state);

    void raiseChannelConnection(int index);

    void lowerChannelConnection(int index);

    void swapViews();

    void popView();

    void popAllViews();

    void zoom(const QPoint& from, const QPoint& to);

    void pan(const QPoint& from, const QPoint& to);

private:
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);
    void timerEvent(QTimerEvent* event);
    void resizeEvent(QResizeEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void paintEvent(QPaintEvent* event);
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);
    void focusInEvent(QFocusEvent* event);
    void updateTransform();
    void emitPointerMoved();

    AzimuthLatch* azimuthLatch_;
    HistoryPosition* historyPosition_;
    QBasicTimer updateTimer_;
    QPixmap background_;
    ChannelConnectionList connections_;
    QSet<QString> connectionNames_;
    std::vector<ViewSettings> viewStack_;
    QMatrix transform_;
    QMatrix inverseTransform_;
    ViewChanger* viewChanger_;
    QPoint mouse_;
    bool mouseMoveEventPending_;
    bool needUpdate_;
    bool frozen_;
    bool showGrid_;
    bool showPeakBars_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
