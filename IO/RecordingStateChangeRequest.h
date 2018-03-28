#ifndef SIDECAR_IO_RECORDINGSTATECHANGEREQUEST_H // -*- C++ -*-
#define SIDECAR_IO_RECORDINGSTATECHANGEREQUEST_H

#include <string>

#include "IO/ControlMessage.h"

namespace SideCar {
namespace IO {

/** Derivation of ControlMessage that commands IO::Task instances to start or stop recording. Currently, only
    the Algorithms::Controller class supports recording, via its Algorithms::Recorder object. The same message
    is used to start and stop a recording. The only difference between the two is that a start request contains
    a fully-qualified path of a directory into which recording files are created.
*/
class RecordingStateChangeRequest : public ControlMessage {
public:
    /** Constructor for a new control message.

        \param path the path of the recording directory, or an empty string to stop a recording
    */
    RecordingStateChangeRequest(const std::string& path);

    /** Constructor for a control message obtained from the network.

        \param data XML-RPC data containing the message payload
    */
    RecordingStateChangeRequest(ACE_Message_Block* data);

    /** Determine if this is a start command

        \return true if a start command
    */
    bool isOn() const { return path_.size() != 0; }

    /** Obtain the recording path

        \return recording path if isOn() is true; otherwise it is an empty string.
    */
    const std::string& getValue() const { return path_; }

private:
    std::string path_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
