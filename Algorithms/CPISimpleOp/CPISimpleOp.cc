#include "boost/bind.hpp"

#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "CPISimpleOp.h"
#include "CPISimpleOp_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

static const char* kOperatorNames[] = {"Sum", "Product", "Min", "Max", "AND", "OR", "NOT"};

const char* const*
CPISimpleOp::OperatorEnumTraits::GetEnumNames()
{
    return kOperatorNames;
}

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur in the
// startup() method. NOTE: it is WRONG to call any virtual functions here...
//
CPISimpleOp::CPISimpleOp(Controller& controller, Logger::Log& log) :
    Super(controller, log, kDefaultEnabled, kDefaultCpiSpan), dataType_(CPISimpleOp::kUnknownType),
    operator_(OperatorParameter::Make("operator", "Operator", Operator(kDefaultOperator)))
{
    ;
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the CPISimpleOp
// class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke Algorithm::startup() as
// shown below.
//
bool
CPISimpleOp::startup()
{
    static Logger::ProcLog log("startup", getLog());
    const IO::Channel& channel(getController().getInputChannel(0));

    if (channel.getTypeKey() == Messages::Video::GetMetaTypeInfo().getKey()) {
        dataType_ = CPISimpleOp::kVideo;
    } else if (channel.getTypeKey() == Messages::BinaryVideo::GetMetaTypeInfo().getKey()) {
        dataType_ = CPISimpleOp::kBinaryVideo;
    } else {
        LOGERROR << "Unknown type of Channel!" << std::endl;
        dataType_ = CPISimpleOp::kUnknownType;
        return false;
    }

    bool ok = true;
    ok = ok && registerParameter(operator_);
    return ok && Super::startup();
}

bool
CPISimpleOp::shutdown()
{
    // Release memory and other resources here.
    //
    return Super::shutdown();
}

bool
CPISimpleOp::cpiSpanChanged(const Parameter::PositiveIntValue& parameter)
{
    return true;
}

/** Verify that the new operator setting is compatible with the type of messages we are processing.
 */
bool
CPISimpleOp::operatorChanged(const OperatorParameter& parameter)
{
    static Logger::ProcLog log("operatorChanged", getLog());
    const IO::Channel& channel(getController().getInputChannel(0));

    if (channel.getTypeKey() == Messages::Video::GetMetaTypeInfo().getKey()) {
        // Make sure the new operator setting applies to this type of message
        if (parameter.getValue() > CPISimpleOp::kLastVideoOp) {
            LOGERROR << "Invalid operator setting.  Not compatible with Video messages" << std::endl;
            return false;
        }
    } else if (channel.getTypeKey() == Messages::BinaryVideo::GetMetaTypeInfo().getKey()) {
        if (parameter.getValue() < CPISimpleOp::kLastVideoOp) {
            LOGERROR << "Invalid operator setting.  Not compatible with BinaryVideo messages" << std::endl;
            // parameter.setValue(CPISimpleOp::kLastVideoOp + 1);
        }
        return false;
    } else {
        LOGERROR << "Unknown type of Channel!" << std::endl;
        return false;
    }

    return true;
}

bool
CPISimpleOp::processVideoCPI()
{
    static Logger::ProcLog log("processVideoCPI", getLog());
    Messages::Video::Ref ref = boost::dynamic_pointer_cast<Messages::Video>(buffer_.front());
    Messages::Video::Ref out = Messages::Video::Make(getName(), ref);

    MessageQueue::iterator pris = buffer_.begin();
    Messages::Video::iterator pos;

    Messages::Video::iterator begin = out->begin();
    Messages::Video::iterator end = out->end();
    Messages::Video::iterator src;
    Messages::Video::iterator last;

    switch (operator_->getValue()) {
    case CPISimpleOp::kSumOp:

        out->resize(ref->size(), 0);

        for (pris = buffer_.begin(); pris != buffer_.end(); pris++) {
            ref = boost::dynamic_pointer_cast<Messages::Video>(*pris);
            src = ref->begin();
            last = ref->end();
            pos = out->begin();
            while (pos != end && src != last) *pos++ += *src++;
        }
        break;

    case CPISimpleOp::kProdOp:

        out->resize(ref->size(), 1);

        for (pris = buffer_.begin(); pris != buffer_.end(); pris++) {
            ref = boost::dynamic_pointer_cast<Messages::Video>(*pris);
            src = ref->begin();
            last = ref->end();
            pos = out->begin();
            while (pos != end && src != last) *pos++ *= *src++;
        }
        break;

    case CPISimpleOp::kMinOp:

        out->getData() = ref->getData();

        for (pris = buffer_.begin(); pris != buffer_.end(); pris++) {
            ref = boost::dynamic_pointer_cast<Messages::Video>(*pris);
            src = ref->begin();
            last = ref->end();
            pos = out->begin();
            while (pos != end && src != last) *pos++ *= std::min(*pos, *src++);
        }
        break;

    case CPISimpleOp::kMaxOp:

        out->getData() = ref->getData();

        for (pris = buffer_.begin(); pris != buffer_.end(); pris++) {
            ref = boost::dynamic_pointer_cast<Messages::Video>(*pris);
            src = ref->begin();
            last = ref->end();
            pos = out->begin();
            while (pos != end && src != last) *pos++ *= std::max(*pos, *src++);
        }
        break;

    default: LOGERROR << "Trying to process a Video CPI with an illegal operation!" << std::endl;
    }

    return send(out);
}

bool
CPISimpleOp::processBinaryVideoCPI()
{
    static Logger::ProcLog log("processBinaryVideoCPI", getLog());

    Messages::BinaryVideo::Ref ref = boost::dynamic_pointer_cast<Messages::BinaryVideo>(buffer_.front());
    Messages::BinaryVideo::Ref out = Messages::BinaryVideo::Make(getName(), ref);

    MessageQueue::iterator pris = buffer_.begin();
    Messages::BinaryVideo::iterator pos;

    Messages::BinaryVideo::iterator begin = out->begin();
    Messages::BinaryVideo::iterator end = out->end();
    Messages::BinaryVideo::iterator src;
    Messages::BinaryVideo::iterator last;

    switch (operator_->getValue()) {
    case kAndOp:

        out->resize(ref->size(), true);

        for (pris = buffer_.begin(); pris != buffer_.end(); pris++) {
            ref = boost::dynamic_pointer_cast<Messages::BinaryVideo>(*pris);
            src = ref->begin();
            last = ref->end();
            pos = out->begin();
            while (pos != end && src != last) *pos++ = *pos && *src++;
        }
        break;

    case kOrOp:

        out->resize(ref->size(), true);

        for (pris = buffer_.begin(); pris != buffer_.end(); pris++) {
            ref = boost::dynamic_pointer_cast<Messages::BinaryVideo>(*pris);
            src = ref->begin();
            last = ref->end();
            pos = out->begin();
            while (pos != end && src != last) *pos++ = *pos || *src++;
        }
        break;

    default: LOGERROR << "Trying to process a Video CPI with an illegal operation!" << std::endl;
    }

    return send(out);
}

bool
CPISimpleOp::processCPI()
{
    static Logger::ProcLog log("processCPI", getLog());

    switch (dataType_) {
    case CPISimpleOp::kVideo: return processVideoCPI();
    case CPISimpleOp::kBinaryVideo: return processBinaryVideoCPI();
    case CPISimpleOp::kUnknownType:
        LOGERROR << "Attempting to process a CPI of unknown type!" << std::endl;
        return false;
    default: LOGERROR << "Attempting to process a CPI of unknown type!" << std::endl; return false;
    };
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (!status[CPIAlgorithm::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    return NULL;
}

// Factory function for the DLL that will create a new instance of the CPISimpleOp class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
CPISimpleOpMake(Controller& controller, Logger::Log& log)
{
    return new CPISimpleOp(controller, log);
}
