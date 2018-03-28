#include "ParametersChangeRequest.h"

using namespace SideCar::IO;

ParametersChangeRequest::ParametersChangeRequest(const XmlRpc::XmlRpcValue& request, bool originalValues) :
    ControlMessage(kParametersChange, sizeof(XmlRpc::XmlRpcValue) + sizeof(int32_t)), originalValues_(originalValues)
{
    // In the ACE_Message_Block that holds the data, we use the first 4 bytes to hold the value of
    // originalValues_ flag.
    //
    int32_t* ptr = new (getData()->wr_ptr()) int32_t;
    *ptr = originalValues_ ? -1 : 0;

    // Move the 'write' pointer passed the int32_t value.
    //
    getData()->wr_ptr(sizeof(int32_t));

    // Now store the XmlRpcValue struct. Use placement new to properly initialize the memory in the
    // ACE_Message_Block.
    //
    new (getData()->wr_ptr()) XmlRpc::XmlRpcValue(request);

    // Move the 'write' pointer passed the XmlRpcValue object.
    //
    getData()->wr_ptr(sizeof(XmlRpc::XmlRpcValue));
}

ParametersChangeRequest::ParametersChangeRequest(ACE_Message_Block* data) : ControlMessage(data)
{
    // Fetch the originalValues_ value from the ACE_Message_Block.
    //
    int32_t* ptr = reinterpret_cast<int32_t*>(getData()->rd_ptr());
    originalValues_ = *ptr ? true : false;

    // Move the 'read' pointer passed the int32_t value.
    //
    getData()->rd_ptr(sizeof(int32_t));
}

const XmlRpc::XmlRpcValue&
ParametersChangeRequest::getValue() const
{
    return *reinterpret_cast<XmlRpc::XmlRpcValue*>(getData()->rd_ptr());
}
