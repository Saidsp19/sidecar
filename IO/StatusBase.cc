#include "StatusBase.h"

using namespace SideCar::IO;

StatusBase::StatusBase(size_t numSlots, const std::string& className, const std::string& name) : status_()
{
    Make(status_, numSlots, className, name);
}

void
StatusBase::Make(XmlRpc::XmlRpcValue& status, size_t numSlots, const std::string& className, const std::string& name)
{
    status.setSize(numSlots);
    status[kVersion] = GetCompiledVersion();
    status[kClassName] = className;
    status[kName] = name;
}
