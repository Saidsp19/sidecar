#include "QtGui/QAction"
#include "QtGui/QPixmap"

#include "ui_ColorMapWidget.h"

#include "CLUT.h"
#include "ColorMapWidget.h"
#include "VideoSampleCountTransform.h"

using namespace SideCar::GUI;

struct ColorMapWidget::GUI : public Ui::ColorMapWidget {
};

ColorMapWidget::ColorMapWidget(VideoSampleCountTransform* transform, QWidget* parent) :
    Super(parent), gui_(new GUI), transform_(transform), normalPalette_(), dbPalette_()
{
    gui_->setupUi(this);

    normalPalette_ = gui_->min_->palette();
    dbPalette_ = normalPalette_;
    dbPalette_.setColor(QPalette::ButtonText, Qt::red);

    decibelStateChange(transform_->getShowDecibels());
    connect(transform_, SIGNAL(settingChanged()), SLOT(updateLabels()));
    connect(transform_, SIGNAL(showingDecibels(bool)), SLOT(decibelStateChange(bool)));
    connect(gui_->db_, SIGNAL(toggled(bool)), transform_, SLOT(setShowDecibels(bool)));

    for (int index = 0; index < CLUT::kNumTypes; ++index) {
        QString name = CLUT::GetTypeName(CLUT::Type(index));
        QAction* action = new QAction(name, this);
        action->setData(index);
        connect(action, SIGNAL(triggered()), SLOT(emitChangeColorMapType()));
        addAction(action);
    }

    setContextMenuPolicy(Qt::ActionsContextMenu);
}

void
ColorMapWidget::setColorMap(const QImage& colorMap)
{
    gui_->colorMap_->setPixmap(QPixmap::fromImage(colorMap));
    if (colorMap.width() != gui_->colorMap_->minimumWidth()) gui_->colorMap_->setMinimumWidth(colorMap.width());
    if (colorMap.width() != gui_->colorMap_->maximumWidth()) gui_->colorMap_->setMaximumWidth(colorMap.width());
}

void
ColorMapWidget::emitChangeColorMapType()
{
    QAction* action = qobject_cast<QAction*>(sender());
    emit changeColorMapType(action->data().toInt());
}

void
ColorMapWidget::decibelStateChange(bool state)
{
    gui_->db_->setChecked(state);
    QPalette p(state ? dbPalette_ : normalPalette_);
    gui_->min_->setPalette(p);
    gui_->q1_->setPalette(p);
    gui_->q2_->setPalette(p);
    gui_->q3_->setPalette(p);
    gui_->max_->setPalette(p);
    updateLabels();
}

void
ColorMapWidget::updateLabels()
{
    gui_->min_->setText(QString::number(transform_->inverseTransform(0.0)));
    gui_->q1_->setText(QString::number(transform_->inverseTransform(0.25)));
    gui_->q2_->setText(QString::number(transform_->inverseTransform(0.50)));
    gui_->q3_->setText(QString::number(transform_->inverseTransform(0.75)));
    gui_->max_->setText(QString::number(transform_->inverseTransform(1.0)));
}

void
ColorMapWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    int x = event->x();
    if (x < size().width() * 0.20) {
        int low = transform_->getSampleMin();
        int value = int(::rint(transform_->getThresholdMin()));
        int high = int(::rint(transform_->getThresholdMax()));
        bool ok = false;
        value = QInputDialog::getInteger(window(), "Min Value", "Set lower cutoff value: ", value, low, high, 1, &ok);
        if (ok) transform_->setThresholdMin(value);
        event->accept();
    } else if (x > size().width() * 0.80) {
        int low = int(::rint(transform_->getThresholdMin()));
        int value = int(::rint(transform_->getThresholdMax()));
        int high = transform_->getSampleMax();
        bool ok = false;
        value = QInputDialog::getInteger(window(), "Max Value", "Set upper cutoff value: ", value, low, high, 1, &ok);
        if (ok) transform_->setThresholdMax(value);
        event->accept();
    } else {
        event->ignore();
        Super::mouseDoubleClickEvent(event);
    }
}
