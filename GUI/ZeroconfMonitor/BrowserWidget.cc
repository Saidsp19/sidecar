#include "GUI/ServiceEntry.h"
#include "BrowserWidget.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::ZeroconfMonitor;

BrowserWidget::BrowserWidget(QWidget* parent)
    : QListWidget(parent), browser_(0), label_(0)
{
    setSortingEnabled(true);
    setAlternatingRowColors(true);
}

void
BrowserWidget::setLabel(QLabel* label)
{
    title_ = label->text();
    label_ = label;
    updateLabelCounts();
}

void
BrowserWidget::updateLabelCounts()
{
    if (label_)
	label_->setText(QString("%1 - %2").arg(title_).arg(count()));
}

void
BrowserWidget::start(const QString& type)
{
    browser_ = new ServiceBrowser(this, type);
    connect(browser_, SIGNAL(foundServices(const ServiceEntryList&)),
            SLOT(foundServices(const ServiceEntryList&)));
    connect(browser_, SIGNAL(lostServices(const ServiceEntryList&)),
            SLOT(lostServices(const ServiceEntryList&)));
    browser_->start();
}

void
BrowserWidget::foundServices(const ServiceEntryList& services)
{
    for (int index = 0; index < services.count(); ++index) {
	ServiceEntry* service = services[index];
	QListWidgetItem* item = new QListWidgetItem(service->getName(), this);
	item->setToolTip(service->getName());
	item->setForeground(Qt::red);
	addItem(item);
	connect(service, SIGNAL(resolved(ServiceEntry*)),
                SLOT(resolved(ServiceEntry*)));
	service->resolve();
    }

    updateLabelCounts();
}

void
BrowserWidget::lostServices(const ServiceEntryList& services)
{
    for (int index = 0; index < services.count(); ++index) {
	ServiceEntry* service = services[index];
	for (int row = 0; row < count(); ++row) {
	    if (item(row)->toolTip() == service->getName()) {
		delete takeItem(row);
		break;
	    }
	}
    }

    updateLabelCounts();
}

void
BrowserWidget::resolved(ServiceEntry* service)
{
    for (int row = 0; row < count(); ++row) {
	QListWidgetItem* item = this->item(row);
	if (item->toolTip() == service->getName()) {
	    item->setForeground(Qt::black);
	    item->setText(QString("%1 [%2/%3]").arg(service->getName())
                          .arg(service->getHost())
                          .arg(service->getPort()));
	}
    }
}
