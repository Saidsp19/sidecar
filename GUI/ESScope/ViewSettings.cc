#include "RadarSettings.h"
#include "ViewSettings.h"

using namespace SideCar::GUI::ESScope;

ViewSettings::ViewSettings(QObject* parent, const RadarSettings* radarSettings,
                           const ViewBounds& initView)
    : QObject(parent), radarSettings_(radarSettings), stack_(),
      home_(initView)
{
    stack_.push_back(home_);
    connect(radarSettings, SIGNAL(settingChanged()),
            SLOT(radarSettingsChanged()));
}

void
ViewSettings::push(const ViewBounds& settings)
{
    stack_.push_back(settings);
    emit viewChanged();
}

void
ViewSettings::pop()
{
    if (stack_.empty()) return;
    stack_.pop_back();
    if (stack_.empty())
	stack_.push_back(home_);
    emit viewChanged();
}

void
ViewSettings::popAll()
{
    stack_.clear();
    stack_.push_back(home_);
    emit viewChanged();
}

void
ViewSettings::swap()
{
    if (stack_.size() > 1) {
	std::swap(stack_.back(), stack_[stack_.size() - 2]);
	emit viewChanged();
    }
}

void
ViewSettings::radarSettingsChanged()
{
    bool notify = false;
    for (size_t index = 0; index < stack_.size(); ++index)
	notify = updateViewBounds(stack_[index]);
    if (notify)
	emit viewChanged();
}

void
ViewSettings::setViewBounds(const ViewBounds& viewBounds)
{
    home_ = viewBounds;
    stack_.clear();
    stack_.push_back(home_);
    emit viewChanged();
}
