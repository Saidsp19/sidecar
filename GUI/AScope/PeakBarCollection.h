#ifndef SIDECAR_GUI_ASCOPE_PEAKBARCOLLECTION_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_PEAKBARCOLLECTION_H

#include "QtCore/QObject"
#include "QtCore/QVector"

#include "Messages/Video.h"

#include "PeakBar.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

namespace AScope {

class PeakBarCollection : public QObject {
    Q_OBJECT
    using Super = QObject;

public:
    static Logger::Log& Log();

    PeakBarCollection(QObject* parent = 0);

    void update(const Messages::PRIMessage::Ref& msg);

    const PeakBar& operator[](int index) const { return peakBars_[index]; }

    const QVector<PeakBar>& getPeakBars() const { return peakBars_; }

    bool isEnabled() const { return enabled_; }

signals:

    void gateTransformChanged();

    void barCountChanged(int size);

    void barValuesChanged(const QVector<int>& changed);

private slots:

    void widthChanged(int value);

    void lifeTimeChanged(int value);

    void enabledChanged(bool value);

private:
    void adjustBars(bool resetBars);

    QVector<PeakBar> peakBars_;
    QVector<int> changed_;
    Messages::Video::Ref last_;
    int width_;
    bool enabled_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

#endif
