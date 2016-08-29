#ifndef SIDECAR_ALGORITHMS_PROCESSOR_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_PROCESSOR_H

#include <vector>
#include "boost/function.hpp"

#include "Messages/Header.h"

namespace SideCar {
namespace Algorithms {

/** Abstract base class that defines the API for all SideCar message processors. By having a common base class,
    Processor objects may reside in STL containers. Although any class may derive from this one, its primary
    purpose is to serve as the base class for the TProcessor template class.
*/
class Processor
{
public:
    
    /** Destructor. Here to silence irritating warnings from certain GNU compilers.
     */
    virtual ~Processor() {}
    
    /** Process a single SideCar message. Derived classes must define.

        \param msg the message to process

        \return true if successful, false if error
    */
    virtual bool process(const Messages::Header::Ref& msg) = 0;
};

/** Templated class that provides type-safe processing of SideCar messages.
 */
template <typename P, typename M>
class TProcessor : public Processor
{
public:

    /** Type definition for a method of class P that can process messages of class M.
     */
    using Proc = boost::function<bool(P*,typename M::Ref)>;

    /** Constructor. Initializes with the given parameters

        \param obj instance of class P that will process incoming messages

        \param proc boost::function wrapper around a method of class P that
        will process incoming messages
    */
    TProcessor(P* obj, Proc proc) : obj_(obj), proc_(proc) {}

    /** Implementation of the Processor base class method. Converts from a generic message type into an
        algorithm-specified message type, and invokes the registered processor method on the message

        \param msg the incoming SideCar message to process

        \return true if successful, false otherwise
    */
    bool process(const Messages::Header::Ref& msg) { return proc_(obj_, boost::dynamic_pointer_cast<M>(msg)); }

private:
    P* obj_;
    Proc proc_;
};

/** Type definition of a STL vector of Processor objects.
 */
using ProcessorVector = std::vector<Processor*>;

} // end namespace Algorithms
} // end namespace SideCar

#endif
