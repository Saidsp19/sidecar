#include "QtCore/QSignalMapper"

#include "Logger/Log.h"

#include "ChannelInfoWidget.h"
#include "ChannelSetting.h"

#include "ui_ChannelInfoWidget.h"

using namespace SideCar::GUI;

ChannelInfoWidget::ChannelInfoWidget(QWidget* parent) :
    Super(parent), gui_(new Ui::ChannelInfoWidget), mapper_(new QSignalMapper(this)), values_()
{
    gui_->setupUi(this);
    // gui_->value_->setTextFormat(Qt::PlainText);
    connect(mapper_, SIGNAL(mapped(int)), SLOT(channelChanged(int)));
    update();
}

void
ChannelInfoWidget::addChannel(const QString& tag, ChannelSetting* channelSetting)
{
    values_.append(tag);
    connect(channelSetting, SIGNAL(valueChanged(int)), mapper_, SLOT(map()));
    int index = values_.size();
    values_.append("N/A");
    mapper_->setMapping(channelSetting, index);
    setTagValue(index, channelSetting);
}

void
ChannelInfoWidget::channelChanged(int index)
{
    ChannelSetting* channelSetting = qobject_cast<ChannelSetting*>(mapper_->mapping(index));
    setTagValue(index, channelSetting);
}

void
ChannelInfoWidget::update()
{
    gui_->value_->setText(values_.join(" "));
}

void
ChannelInfoWidget::setTagValue(int index, const ChannelSetting* channelSetting)
{
    QString value;
    if (!channelSetting->hasChannels()) {
        value = "N/A";
    } else {
        value = channelSetting->getValue();
        if (value.trimmed().size() == 0) value = "-";
    }

    values_[index] = value;

    update();
}
