#include "QtWidgets/QGridLayout"

#include "GUI/LogUtils.h"

#include "App.h"
#include "Configuration.h"
#include "GridImaging.h"
#include "ScaleWidget.h"
#include "ViewSettings.h"
#include "XYView.h"
#include "XYWidget.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::ESScope;

Logger::Log&
XYView::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("esscope.XYView");
    return log_;
}

XYView::XYView(QWidget* parent, ViewSettings* viewSettings, const QString& title, const QString& xLabel,
               const QString& yLabel) :
    Super(parent),
    radarSettings_(App::GetApp()->getConfiguration()->getRadarSettings()), viewSettings_(viewSettings), display_(0),
    title_(title), xLabel_(xLabel), yLabel_(yLabel), heading_(0), xScale_(0), yScale_(0), cursorX_(0.0), cursorY_(0.0)
{
    connect(viewSettings, SIGNAL(viewChanged()), SLOT(viewSettingsChanged()));

    QGridLayout* layout = new QGridLayout(this);
    layout->setSpacing(0);
    layout->setMargin(0);

    heading_ = new QLabel(title, this);
    heading_->setAlignment(Qt::AlignCenter);
    layout->addWidget(heading_, 0, 0, 1, 2, Qt::AlignHCenter);

    yScale_ = new ScaleWidget(this, Qt::Vertical);
    yScale_->setLabel(yLabel);
    yScale_->setAutoDivide(true);
    layout->addWidget(yScale_, 1, 0);

    xScale_ = new ScaleWidget(this, Qt::Horizontal);
    xScale_->setLabel(xLabel);
    xScale_->setAutoDivide(true);
    layout->addWidget(xScale_, 2, 1);

    Configuration* cfg = App::GetApp()->getConfiguration();
    GridImaging* gridImaging = cfg->getGridImaging();
    connect(gridImaging, SIGNAL(settingChanged()), SLOT(updateGridPositions()));
}

void
XYView::setXYWidget(XYWidget* display)
{
    display_ = display;
    static_cast<QGridLayout*>(layout())->addWidget(display_, 1, 1);
    connect(display_, SIGNAL(cursorMoved(double, double)), SLOT(cursorMoved(double, double)));
    viewSettingsChanged();
    updateGridPositions();
}

void
XYView::viewSettingsChanged()
{
    xScale_->setStartAndEnd(viewSettings_->getXMin(), viewSettings_->getXMax());
    yScale_->setStartAndEnd(viewSettings_->getYMax(), viewSettings_->getYMin());
}

void
XYView::cursorMoved(double x, double y)
{
    cursorX_ = x;
    cursorY_ = y;
    heading_->setText(QString("%1: %2  %3: %4").arg(xLabel_).arg(formatXValue(x)).arg(yLabel_).arg(formatYValue(y)));
    xScale_->setCursorValue(x);
    yScale_->setCursorValue(y);
}

void
XYView::updateGridPositions()
{
    static Logger::ProcLog log("updateGridPositions", Log());
    LOGINFO << std::endl;
    display_->setGridPositions(xScale_->getMajorTickPositions(), yScale_->getMajorTickPositions());
}

void
XYView::refresh()
{
    display_->update();
}

void
XYView::clear()
{
    display_->clear();
}

void
XYView::showEvent(QShowEvent* event)
{
    Super::showEvent(event);
    updateGridPositions();
    display_->update();
}

void
XYView::resizeEvent(QResizeEvent* event)
{
    Super::resizeEvent(event);
    updateGridPositions();
    display_->update();
}

void
XYView::setSlaveAlpha(double value)
{
    cursorMoved(value, cursorY_);
}
