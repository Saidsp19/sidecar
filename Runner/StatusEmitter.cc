#include "IO/ZeroconfRegistry.h"

#include "App.h"
#include "RunnerStatus.h"
#include "StatusEmitter.h"

using namespace SideCar::IO;
using namespace SideCar::Runner;

StatusEmitter::StatusEmitter(App& app, double updateRate) :
    StatusEmitterBase(GetEmitterType(), GetCollectorType(), updateRate), app_(app)
{
    ;
}

void
StatusEmitter::fillStatus(XmlRpc::XmlRpcValue& status)
{
    app_.fillStatus(status);
}
