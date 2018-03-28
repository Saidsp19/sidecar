#include "ZeroconfRegistry.h"

using namespace SideCar::IO;

/** NOTE: at one time the kPublisherTCP/kSubscriberTCP and kPublisherUDP/kSubscriberUDP values were different.
    That is no longer the case. Now, the published Zeroconf entry contains a TXT field with the name "transport"
    that contains either "tcp" or "multicast", depending on the publisher doing the publishing.
*/
const char* ZeroconfRegistry::kTypes_[kNumTypes] = {
    "_scPub._tcp",            // kPublisher
    "_scSub._tcp",            // kSubscriber
    "_scRnrSC._udp",          // kRunnerStatusCollector
    "_scRnrSE._udp",          // kRunnerStatusEmitter
    "_scRnrRC._tcp",          // kRunnerRemoteController
    "_scStateEmitter._udp",   // kStateEmitter
    "_scStateCollector._udp", // kStateCollector
};

const char*
ZeroconfRegistry::GetType(ID id)
{
    return kTypes_[id];
}

std::string
ZeroconfRegistry::MakeZeroconfType(const char* type, const std::string& subType)
{
    std::string tmp(type);
    if (!subType.empty()) {
        tmp += ",_"; // !!! Subtypes must begin with an underscore
        tmp += subType;
    }
    return tmp;
}
