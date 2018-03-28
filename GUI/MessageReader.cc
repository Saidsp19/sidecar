#include "IO/MessageManager.h"
#include "Messages/Header.h"
#include "Messages/MetaTypeInfo.h"

#include "LogUtils.h"
#include "MessageReader.h"
#include "MulticastMessageReader.h"
#include "ServiceEntry.h"
#include "TCPMessageReader.h"

using namespace SideCar;
using namespace SideCar::GUI;

Logger::Log&
MessageReader::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.MessageReader");
    return log_;
}

MessageReader*
MessageReader::Make(const ServiceEntry* serviceEntry)
{
    static Logger::ProcLog log("Make", Log());
    LOGINFO << "name: " << serviceEntry->getName() << " host: " << serviceEntry->getHost()
            << " port: " << serviceEntry->getPort() << " transport: " << serviceEntry->getTransport() << std::endl;

    MessageReader* reader = 0;
    if (serviceEntry->getTransport() == "multicast") {
        reader = MulticastMessageReader::Make(serviceEntry);
    } else if (serviceEntry->getTransport() == "tcp") {
        reader = TCPMessageReader::Make(serviceEntry);
    } else {
        LOGERROR << "invalid transport - " << serviceEntry->getTransport() << std::endl;
        return 0;
    }

    if (!reader) {
        LOGERROR << "failed to create reader" << std::endl;
        return 0;
    }

    return reader;
}

MessageReader::MessageReader(const Messages::MetaTypeInfo* metaTypeInfo) : Super(), metaTypeInfo_(metaTypeInfo)
{
    ;
}

void
MessageReader::addRawData(ACE_Message_Block* raw)
{
    static Logger::ProcLog log("addRawData", Log());
    IO::MessageManager mgr(raw, metaTypeInfo_);
    Messages::Header::Ref msg(mgr.getNative());
    emit received(msg);
}
