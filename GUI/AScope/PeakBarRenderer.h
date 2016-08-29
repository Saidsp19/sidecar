#ifndef SIDECAR_GUI_ASCOPE_PEAKBARRENDERER_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_PEAKBARRENDERER_H

#include "QtCore/QLineF"
#include "QtCore/QObject"
#include "QtCore/QPointF"
#include "QtCore/QRectF"
#include "QtCore/QVector"

#include "PeakBar.h"

class QMatrix;
class QPainter;

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace AScope {

class ChannelConnection;
class PeakBarCollection;

/** Rendering class for PeakBarCollection objects.
 */
class PeakBarRenderer : public QObject
{
    Q_OBJECT
    using Super = QObject;
public:

    static Logger::Log& Log();

    PeakBarRenderer(ChannelConnection& channelConnection);

    bool isEnabled() const { return enabled_; }

    bool isDirty() const { return dirty_; }

    void render(QPainter& painter);

    void calculateGeometry(const QMatrix& transform,
                           const Messages::PRIMessage::Ref& msg,
                           bool showRanges, bool showVoltages);

public slots:

    void unfreeze();

    void dirtyCache(bool allDirty = false);

    void gateTransformChanged();

private slots:

    void barCountChanged(int size);

    void barValuesChanged(const QVector<int>& changed);

    void widthChanged(int value);

    void fadingChanged(bool value);

    void enabledChanged(bool value);

private:

    const ChannelConnection& channelConnection_;
    const PeakBarCollection& model_;
    QVector<PeakBar> cache_;
    QVector<QLineF> lines_;
    QVector<QRectF> rects_;

    QSizeF barSize_;
    QPointF origin_;
    int width_;
    bool fading_;
    bool enabled_;
    bool dirty_;
    bool allDirty_;
    bool transformDirty_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

#endif
