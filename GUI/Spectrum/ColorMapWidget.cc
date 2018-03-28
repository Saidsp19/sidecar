#include "QtGui/QContextMenuEvent"
#include "QtGui/QInputDialog"
#include "QtGui/QPixmap"

#include "ui_ColorMapWidget.h"

#include "App.h"
#include "ColorMapWidget.h"
#include "Configuration.h"
#include "SpectrographImaging.h"
#include "VideoSampleCountTransform.h"

using namespace SideCar::GUI::Spectrum;

struct ColorMapWidget::GUI : public Ui::ColorMapWidget {
};

ColorMapWidget::ColorMapWidget(double minCutoff, double maxCutoff, QWidget* parent) :
    Super(parent), gui_(new GUI), minCutoff_(minCutoff), maxCutoff_(maxCutoff)
{
    gui_->setupUi(this);
    updateLabels();
    for (int index = CLUT::kBlueSaturated; index < CLUT::kNumTypes; ++index) {
        QString name = CLUT::GetTypeName(CLUT::Type(index));
        QAction* action = new QAction(name, this);
        action->setData(index);
        connect(action, SIGNAL(triggered()), SLOT(changeColorMap()));
        addAction(action);
    }

    setContextMenuPolicy(Qt::ActionsContextMenu);
}

void
ColorMapWidget::changeColorMap()
{
    QAction* action = qobject_cast<QAction*>(sender());
    App::GetApp()->getConfiguration()->getSpectrographImaging()->setColorMapIndex(action->data().toInt());
}

void
ColorMapWidget::setColorMap(const QImage& colorMap)
{
    gui_->colorMap_->setPixmap(QPixmap::fromImage(colorMap));
}

void
ColorMapWidget::setMinCutoff(double minCutoff)
{
    minCutoff_ = minCutoff;
    updateLabels();
}

void
ColorMapWidget::setMaxCutoff(double maxCutoff)
{
    maxCutoff_ = maxCutoff;
    updateLabels();
}

void
ColorMapWidget::updateLabels()
{
    double range = maxCutoff_ - minCutoff_;
    gui_->min_->setText(QString::number(0.00 * range + minCutoff_) + "dB");
    gui_->q1_->setText(QString::number(0.25 * range + minCutoff_) + "dB");
    gui_->q2_->setText(QString::number(0.50 * range + minCutoff_) + "dB");
    gui_->q3_->setText(QString::number(0.75 * range + minCutoff_) + "dB");
    gui_->max_->setText(QString::number(1.00 * range + minCutoff_) + "dB");
}

void
ColorMapWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    int x = event->x();
    if (x < size().width() * 0.20) {
        bool ok = false;
        double value =
            QInputDialog::getDouble(window(), "Min Cutoff", "Min Cutoff: ", minCutoff_, -200.0, maxCutoff_, 1, &ok);
        if (ok) { App::GetApp()->getConfiguration()->getSpectrographImaging()->setMinCutoff(value); }
        event->accept();
    } else if (x > size().width() * 0.80) {
        bool ok = false;
        double value =
            QInputDialog::getDouble(window(), "Max Cutoff", "Max Cutoff: ", maxCutoff_, minCutoff_, 0.0, 1, &ok);
        if (ok) { App::GetApp()->getConfiguration()->getSpectrographImaging()->setMaxCutoff(value); }
        event->accept();
    } else {
        event->ignore();
        Super::mouseDoubleClickEvent(event);
    }
}
