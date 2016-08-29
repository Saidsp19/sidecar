#include <algorithm>
#include <cmath>
#include <limits>

#include "GUI/LogUtils.h"
#include "PeakBar.h"

using namespace SideCar;
using namespace SideCar::GUI::AScope;

Logger::Log&
PeakBar::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ascope.PeakBar");
    return log_;
}

int PeakBar::lifeTime_ = 10;
QList<double> PeakBar::decayLookup_;

void
PeakBar::SetLifeTime(int lifeTime)
{
    lifeTime_ = lifeTime;
    decayLookup_.clear();

    // Create a log10 lookup table for peak bar decays. The values range from 1.0 (full intensity) to ~0.3 (dim)
    // as the PeakBar age climbs from 0 to lifeTime_.
    //
    for (int index = 0; index <= lifeTime_; ++index) {
	decayLookup_.push_back(::log10(double(index) /
                                       double(lifeTime_) * 8.0 + 2.0));
    }
}

double
PeakBar::GetDecay(int age)
{
    if (decayLookup_.empty())
	SetLifeTime(lifeTime_);
    return decayLookup_[age];
}

bool
PeakBar::update(Messages::Video::const_iterator pos,
                Messages::Video::const_iterator end, bool reset)
{
    static Logger::ProcLog log("update", Log());
    LOGINFO << "reset: " << reset << std::endl;

    if (reset)
	age_ = 0;

    pos = std::max_element(pos, end);
    if (pos == end) {
	LOGDEBUG << "no data for bar" << std::endl;
	return false;
    }

    if (age_ <= 0 || *pos >= value_) {
	value_ = *pos;
	age_ = lifeTime_;
	LOGDEBUG << "new value: " << value_ << " lifeTime: " << age_
		 << std::endl;
    }
    else {
	--age_;
    }

    return age_ == lifeTime_;
}
