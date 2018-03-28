#include "Logger/Log.h"
#include "Messages/Video.h"

#include "IQFilter.h"
#include "IQFilter_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

static const char* kFilterNames[] = {
    "10 * log10(Magnitude)", "Magnitude", "Magnitude ^ 2", "I Only", "Q Only", "Phase Angle - 2000 * ATAN2(I, Q)"};

const char* const*
IQFilter::FilterTypeEnumTraits::GetEnumNames()
{
    return kFilterNames;
}

IQFilter::IQFilter(Controller& controller, Logger::Log& log) :
    Super(controller, log), filterType_(FilterTypeParameter::Make("filter", "Filter", FilterType(kDefaultFilter))),
    enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled))
{
    ;
}

bool
IQFilter::startup()
{
    registerProcessor<IQFilter, Messages::Video>(&IQFilter::process);
    return registerParameter(filterType_) && registerParameter(enabled_) && Super::startup();
}

bool
IQFilter::process(const Messages::Video::Ref& in)
{
    static Logger::ProcLog log("processVideo", getLog());

    if (!enabled_->getValue()) return send(in);

    Messages::Video::Ref out(Messages::Video::Make(getName(), in));
    Messages::Video::Container& outData(out->getData());
    outData.reserve(in->size() / 2);
    const Messages::Video::DatumType* pos = &in[0];
    const Messages::Video::DatumType* end = &in[(in->size() / 2) * 2];

    switch (filterType_->getValue()) {
    case kLogSqrtSumIQSquared:
        while (pos != end) {
            float r = *pos++;
            float i = *pos++;
            outData.push_back(::rintf(10.0 * ::log10f(::sqrtf(r * r + i * i))));
        }
        break;

    case kSqrtSumIQSquared:
        while (pos != end) {
            float r = *pos++;
            float i = *pos++;
            outData.push_back(::rintf(::sqrtf(r * r + i * i)));
        }
        break;

    case kSumIQSquared:
        while (pos != end) {
            float r = *pos++;
            float i = *pos++;
            outData.push_back(::rintf(r * r + i * i));
        }
        break;

    case kISamples:
        while (pos != end) {
            outData.push_back(*pos);
            pos += 2;
        }
        break;

    case kQSamples:
        ++pos;
        ++end;
        while (pos != end) {
            outData.push_back(*pos);
            pos += 2;
        }
        break;

    case kPhaseAngle:
        while (pos != end) {
            float r = *pos++;
            float i = *pos++;
            out->push_back(::rintf(::atan2f(r, i) * 2000.0));
        }
        break;

    default: LOGERROR << "invalid filter type - " << filterType_->getValue() << std::endl; break;
    }

    bool rc = send(out);
    LOGDEBUG << "rc: " << rc << std::endl;

    return rc;
}

void
IQFilter::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kFilterType, filterType_->getValue());
    status.setSlot(kEnabled, enabled_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (!status[IQFilter::kEnabled]) return Algorithm::FormatInfoValue("Disabled");

    int index = status[IQFilter::kFilterType];
    return Algorithm::FormatInfoValue(QString(kFilterNames[index]));
}

extern "C" ACE_Svc_Export Algorithm*
IQFilterMake(Controller& controller, Logger::Log& log)
{
    return new IQFilter(controller, log);
}
