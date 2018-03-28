#include <cmath>
#include <iostream>

#include "GUI/CursorWidget.h"
#include "GUI/LogUtils.h"
#include "Utils/Utils.h"

#include "App.h"
#include "CursorWidget.h"

using namespace SideCar::GUI::PPIDisplay;

const int CursorWidget::kUpdateInterval = 250;

QString
CursorWidget::GetFormattedPosition(const QPointF& pos)
{
    double azimuth = Utils::normalizeRadians(::atan2(pos.x(), pos.y()));
    double range = ::sqrt(pos.x() * pos.x() + pos.y() * pos.y());
    return QString("Az: %1 Range: %2")
        .arg(App::GetApp()->getFormattedAngleRadians(azimuth))
        .arg(App::GetApp()->getFormattedDistance(range));
}

CursorWidget::CursorWidget(QWidget* parent) : Super(parent), pos_(), updateTimer_(), update_(true)
{
    connect(App::GetApp(), SIGNAL(distanceUnitsChanged(const QString&)), SLOT(refresh()));
    connect(App::GetApp(), SIGNAL(angleFormattingChanged(int)), SLOT(refresh()));
    refresh();
}

void
CursorWidget::showCursorPosition(const QPointF& pos)
{
    if (pos_ != pos) {
        pos_ = pos;
        update_ = true;
        if (!updateTimer_.isActive()) {
            updateTimer_.start(kUpdateInterval, this);
            refresh();
        }
    }
}

void
CursorWidget::refresh()
{
    setValue(GetFormattedPosition(pos_));
}

void
CursorWidget::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == updateTimer_.timerId()) {
        if (update_) {
            update_ = false;
            refresh();
        } else {
            updateTimer_.stop();
        }
    } else {
        Super::timerEvent(event);
    }
}
