#ifndef SIDECAR_GUI_RTCLMESSAGEREADER_H // -*- C++ -*-
#define SIDECAR_GUI_RTCLMESSAGEREADER_H

#include "IO/Task.h"
#include "GUI/MessageReader.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

/** Derivation of MessageReader that obtains messages over an RTCL topic. We derived from ACE_Task in order that
    we can connect to our held RTCLSubscriber object so that we will receive incoming messages from it.
*/
class RTCLMessageReader : public MessageReader, private ACE_Task<ACE_MT_SYNCH>
{
    Q_OBJECT
    using Super = MessageReader;
public:

    /** Obtain the log device for RTCLMessageReader objects.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory class method that creates new RTCLMessageReader objects.
     */
    static RTCLMessageReader* Make(const std::string& topic, const Messages::MetaTypeInfo* type);

private:

    /** Constructor.
     */
    RTCLMessageReader(const Messages::MetaTypeInfo* type, const IO::Task::Ref& reader); 

    int put(ACE_Message_Block* data, ACE_Time_Value* timeout = 0);

    IO::Task::Ref reader_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
