#ifndef SIDECAR_IO_STATUSBASE_H // -*- C++ -*-
#define SIDECAR_IO_STATUSBASE_H

#include <iostream>

#include "IO/Printable.h"
#include "IO/ProcessingState.h"
#include "XMLRPC/XmlRpcValue.h"

namespace SideCar {
namespace IO {

/** Base class for all status collections. A status collection is a vector of XML-RPC values that is sent out by
    a StatusEmitter object to one or more listening StatusCollector objects.

    The base collection contains the class name of the status collection, and the name of the object that filled
    in the status.
*/
class StatusBase : public Printable<StatusBase> {
public:
    /** Indices for status elements managed by this class.
     */
    enum Index { kVersion = 0, kClassName, kName, kNumSlots };

    static std::string GetCompiledVersion() { return "2.0"; }

    static void Make(XmlRpc::XmlRpcValue& status, size_t numSlots, const std::string& className,
                     const std::string& name);

    StatusBase() : status_() {}

    /**
     */
    StatusBase(size_t numSlots, const std::string& className, const std::string& name);

    /** Conversion constructor. Acquires status values from an XML-RPC container.

        \param status values to acquire
    */
    StatusBase(const XmlRpc::XmlRpcValue& status) : status_(status) {}

    /** Creates an XML value from the internal data.

        \return
    */
    const XmlRpc::XmlRpcValue& getXMLData() const { return status_; }

    /** Determine if the held value is valid.

        \return true if so
    */
    bool isValid() const { return status_.valid(); }

    /** Obtain a status value at a particular index.

        \param index location to fetch

        \return read-only reference to found value
    */
    const XmlRpc::XmlRpcValue& getSlot(size_t index) const { return status_[index]; }

    /** Obtain a status value at a particular index.

        \param index location to fetch

        \return read-only reference to found value
    */
    void setSlot(size_t index, const XmlRpc::XmlRpcValue& value) { status_[index] = value; }

    std::string getVersion() const { return getSlot(kVersion); }

    bool isLatestVersion() const { return getVersion() == GetCompiledVersion(); }

    /** Obtain the class name held in the status container.

        \return class name
    */
    std::string getClassName() const { return getSlot(kClassName); }

    /** Obtain the object name held in the status container.

        \return object name
    */
    std::string getName() const { return getSlot(kName); }

    const XmlRpc::XmlRpcValue& operator[](size_t index) const { return getSlot(index); }

    /** Determine if the held status information is the same as that in another status container

        \param rhs value to compare with

        \return true if so
    */
    bool operator==(const StatusBase& rhs) const { return status_ == rhs.status_; }

    /** Determine if the held status information is not the same as that in another status container

        \param rhs value to compare with

        \return true if so
    */
    bool operator!=(const StatusBase& rhs) const { return !operator==(rhs.status_); }

    /** Print out an XML representation of the held status values.

        \param os stream to write to

        \return stream written to.
    */
    std::ostream& print(std::ostream& os) const { return os << status_.toXml(); }

private:
    XmlRpc::XmlRpcValue status_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
