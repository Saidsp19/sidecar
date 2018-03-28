#ifndef SIDECAR_PROPERTYMONITOR_H // -*- C++ -*-
#define SIDECAR_PROPERTYMONITOR_H

#include "boost/shared_ptr.hpp"
#include <string>

namespace SideCar {
namespace Parameter {

/** Monitor and apply changes to a Value instance. The setValue() method will apply a change to the associated
    Value instance, and will invoke the valueChanged() notification method, which derived classes must define.
*/
template <typename Parameter>
class Monitor {
public:
    using Self = Monitor<Parameter>;

    /** Type to use for shared references to Monitor objects.
     */
    using Ref = boost::shared_ptr<Self>;

    /** Constructor. Associate with a specific Value instance.

        \param value Value istance to monitor
    */
    Monitor(typename Parameter::Ref& parameter) :
        connection_(parameter->connectChangedSignalTo([this](auto& v) { valueChanged(v); }))
    {
    }

    /** Destructor. Disconnect monitor from the parameter's change signal.
     */
    virtual ~Monitor()
    {
        if (connection_.connected()) connection_.disconnect();
    }

protected:
    /** Event handler called after our associated value has changed. Derived classes must define.

        \param value parameter that has changed. NOTE: do not save a copy of
        this value; it is only valid for the duration of the valueChanged()
        call.
    */
    virtual void valueChanged(const Parameter& value) = 0;

private:
    typename Parameter::SignalConnection connection_;
};

/** Proxy version of the Monitor class that forwards forwards valueChanged notifications to another object and
    method.
*/
template <typename Parameter, typename Destination>
class MonitorProxy : public Monitor<Parameter> {
public:
    using Self = MonitorProxy<Parameter, Destination>;

    /** Type to use for shared references to MonitorProxy objects.
     */
    using Ref = boost::shared_ptr<Self>;

    /** Type of the destination object we accept in the constructor. We require that it be a boost::shared_ptr
        reference so that we can detect when it is no longer valid.
    */
    using ObjRef = boost::shared_ptr<Destination>;

    /** Type definition for the method invoked from valueChanged().
     */
    using Proc = void (Destination::*)(const Parameter& value);

    /** Constructor.

        \param value Value instance to monitor

        \param obj object to receive notifications

        \param p procedure of the object to call
    */
    MonitorProxy(typename Parameter::Ref& parameter, ObjRef obj, Proc p) :
        Monitor<Parameter>(parameter), obj_(obj), proc_(p)
    {
    }

private:
    /** Event handler called after our associated value has changed. Forward notification to stored
        object/procedure.

        \param value parameter that has changed
    */
    void valueChanged(const Parameter& value)
    {
        if (!obj_.expired()) {
            ObjRef ref(obj_.lock());
            ((ref.get())->*proc_)(value);
        }
    }

    boost::weak_ptr<Destination> obj_; ///< Object to receive notifications
    Proc proc_;                        ///< Method to call for notifications
};

} // namespace Parameter
} // end namespace SideCar

/** \file
 */

#endif
