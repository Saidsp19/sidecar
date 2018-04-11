#include "BinaryOp.h"
#include "Algorithms/Utils.h"
#include "BinaryOp_defaults.h"
#include "Logger/Log.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

static const char* kOperatorNames[] = {
    "AND",
    "OR",
    "XOR",
    "NOT"
    "NAND",
    "NOR",
    "NXOR",
};

const char* const*
BinaryOp::OperatorEnumTraits::GetEnumNames()
{
    return kOperatorNames;
}

BinaryOp::BinaryOp(Controller& controller, Logger::Log& log) :
    Super(controller, log, kDefaultEnabled, kDefaultMaxBufferSize),
    operator_(OperatorParameter::Make("operator", "Operator", Operator(kDefaultOperator)))
{
    ;
}

bool
BinaryOp::startup()
{
    return Super::startup() && registerParameter(operator_);
}

namespace {

struct BinaryProc
    : public std::binary_function<BinaryVideo::DatumType, BinaryVideo::DatumType, BinaryVideo::DatumType> {
    using Arg = first_argument_type;
};

struct UnaryProc : public std::unary_function<BinaryVideo::DatumType, BinaryVideo::DatumType> {
    using Arg = argument_type;
};

/** Functor that performs boolean AND operation.
 */
struct AndOp : public BinaryProc {
    Arg operator()(Arg a, Arg b) const { return a && b; }
};

/** Functor that performs boolean OR operation.
 */
struct OrOp : public BinaryProc {
    Arg operator()(Arg a, Arg b) const { return a || b; }
};

/** Functor that performs boolean XOR operation.
 */
struct ExclusiveOrOp : public BinaryProc {
    Arg operator()(Arg a, Arg b) const { return a ^ b; }
};

/** Functor that performs boolean NOT operation.
 */
struct NotOp : public UnaryProc {
    Arg operator()(Arg a) const { return !a; }
};

} // namespace

bool
BinaryOp::processChannels()
{
    static Logger::ProcLog log("processInput", getLog());
    LOGINFO << std::endl;

    using MsgVector = std::vector<BinaryVideo::Ref>;
    MsgVector inputs;
    size_t minSize = fetchEnabledChannelMessages<BinaryVideo>(inputs);

    // If we are not enabled, just copy the message from the first enabled channel to the output.
    //
    if (!isEnabled()) {
        LOGDEBUG << "disabled" << std::endl;
        if (!inputs.empty()) {
            BinaryVideo::Ref msg(inputs.front());
            BinaryVideo::Ref out(BinaryVideo::Make(getName(), msg));
            out->getData() = msg->getData();
            return send(out);
        }

        LOGDEBUG << "nothing to output" << std::endl;
        return true;
    } else if (inputs.empty()) {
        LOGWARNING << "enabled but no channels enabled" << std::endl;
        return true;
    }

    // Create an output message using the first input message as its basis.
    //
    BinaryVideo::Ref out(BinaryVideo::Make(getName(), inputs.front()));
    out->resize(minSize);

    Operator op = operator_->getValue();
    LOGDEBUG << "operation " << op << ' ' << kOperatorNames[op] << std::endl;

    // We support configurations with more than two inputs. To do so, we perform the binary operation on the
    // first two inputs and store the result in the output message. For subsequent passes, we perform the binary
    // operation on the output message and the next input message, storing the result back in the output
    // message. This works fine, except that we must hold off performing the NOT operation for the NAND, NOR,
    // NXOR operations until we've processed all operations.
    //
    BinaryVideo::Ref a = inputs.front();
    if (op != kNotOp) {
        for (size_t index = 1; index < inputs.size(); ++index) {
            BinaryVideo::Ref b = inputs[index];
            switch (operator_->getValue()) {
            case kAndOp:
            case kNotAndOp:
                std::transform(a->begin(), a->begin() + minSize, b->begin(), out->begin(), AndOp());
                break;

            case kOrOp:
            case kNotOrOp:
                std::transform(a->begin(), a->begin() + minSize, b->begin(), out->begin(), OrOp());
                break;

            case kXorOp:
            case kNotXorOp:
                std::transform(a->begin(), a->begin() + minSize, b->begin(), out->begin(), ExclusiveOrOp());
                break;

            default: break;
            }

            // Make the next 'a' input be the results of the last binary operation.
            //
            a = out;
        }
    }

    // Need to perform a NOT operation on the result? If we've gone through the above loop at least once, 'a'
    // will alias 'out'.
    //
    if (op >= kNotOp) {
        std::transform(a->begin(), a->begin() + minSize, out->begin(), NotOp());
    }

    LOGDEBUG << out->dataPrinter() << std::endl;

    return send(out);
}

ChannelBuffer*
BinaryOp::makeChannelBuffer(int channelIndex, const std::string& name, size_t maxBufferSize)
{
    BinaryChannelBuffer* channel = new BinaryChannelBuffer(*this, channelIndex, maxBufferSize);
    channel->makeEnabledParameter();
    return channel;
}

void
BinaryOp::setInfoSlots(IO::StatusBase& status)
{
    Super::setInfoSlots(status);
    status.setSlot(kOperator, operator_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;

    if (!status[ManyInAlgorithm::kEnabled])
        return Algorithm::FormatInfoValue(ManyInAlgorithm::GetFormattedStats(status));

    return Algorithm::FormatInfoValue(ManyInAlgorithm::GetFormattedStats(status) +
                                      QString(kOperatorNames[int(status[BinaryOp::kOperator])]));
}

// Factory function for the DLL that will create a new instance of the BinaryOp class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
BinaryOpMake(Controller& controller, Logger::Log& log)
{
    return new BinaryOp(controller, log);
}
