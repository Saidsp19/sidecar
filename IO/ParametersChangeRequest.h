#ifndef SIDECAR_IO_PARAMETERSCHANGEREQUEST_H // -*- C++ -*-
#define SIDECAR_IO_PARAMETERSCHANGEREQUEST_H

#include "IO/ControlMessage.h"
#include "XMLRPC/XmlRpcValue.h"

namespace SideCar {
namespace IO {

/** Control message used to change the value of one or more runtime parameter values used by an algorithm.
    Normally, the Master application creates this message due to the user changing parameter settings in a
    dialog box. The Master application then sends the control message to the Runner::RemoteController XML-RPC
    server, which takes the control message and places it into the message queue of the appropriate
    Algorithm::Controller object. Note that unlike most control (and data) messages, this one gets placed at the
    head of the controller's message queue so that it will take effect immediately.

    The XML-RPC data for a ParametersChangeRequest message consists of an array of name-value pairs, even
    entries containing the name of the parameter to change, and odd entries containing the new value.
*/
class ParametersChangeRequest : public ControlMessage {
public:
    /** Constructor. Used to create a new control message from data contained in an XML-RPC container.

        \param request description of the parameters to change.
    */
    ParametersChangeRequest(const XmlRpc::XmlRpcValue& request, bool originalValues);

    /** Constructor. Used to reconstitute the XML data from data received over a network.

        \param data raw data
    */
    ParametersChangeRequest(ACE_Message_Block* data);

    /** Obtain the XML-RPC data from the request. The returned container is an XML-RCP array of name-value
        pairs.

        \return XML-RPC array
    */
    const XmlRpc::XmlRpcValue& getValue() const;

    bool hasOriginalValues() const { return originalValues_; }

private:
    bool originalValues_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
