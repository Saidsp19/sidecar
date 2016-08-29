#ifndef SIDECAR_IO_ZEROCONFREGISTRY_H // -*- C++ -*-
#define SIDECAR_IO_ZEROCONFREGISTRY_H

#include <string>

namespace SideCar {
namespace IO {

/** Registry of Zeroconf types values used within the SideCar system. Collected here to make it easier to find
    the actual values used; previously, they were scattered around the source tree.
*/
class ZeroconfRegistry
{
public:

    /** Defined Zeroconf types. Entries here should be entered in pairs, at least at the start of the list: the
	'twin' logic below depends on pair diffences existing only in the least significant bit.
    */
    enum ID {
	kPublisher = 0,		 // _scPub._tcp
	kSubscriber,		 // _scSub._tcp
	kRunnerStatusCollector,	 // _scRnrSC._udp
	kRunnerStatusEmitter,	 // _scRnrSE._udp
	kStateEmitter,		 // _scStateEmitter._udp
	kStateCollector,	 // _scStateCollector._udp

	// See above -- keep paired entries above this one.
	//
	kRunnerRemoteController, // _scRnrRC._tcp
	kNumTypes
    };

    /** Obtain the Zeroconf type string for a given ID. NOTE: DO NOT inline this, or the kTypes_ member may not
        initialize properly.

        \param id which type to obtain 

        \return NULL-terminated C string
    */
    static const char* GetType(ID id);

    /** Locate the twin of this ID. A subscriber's twin is its publisher, and a status collector's twin is its
        emitter

        \param id the value to work with

        \return the twin value
    */
    static ID GetTwin(ID id) { return ID(id ^ 1); }

    /** Construct a valid SideCar Zeroconf type string for a type and a subtype.

        \param type the main type to use (eg. _scPub._tcp)

        \param subType the data type restriction (eg. Video)

        \return combined type + subtype (eg. _scPub._tcp,_Video)
    */
    static std::string MakeZeroconfType(const char* type,
                                        const std::string& subType);

private:
    static const char* kTypes_[kNumTypes];
};

/** Collection of Zeroconf types used by SideCar applications and libraries.
 */
namespace ZeroconfTypes
{

/** Template class that adds class methods that return character types for the template parameter type ID as
    well as for its twin ID value. Other classes may derive from this template in order to assign a Zeroconf
    type, and to get methods to access the types.
*/
template <ZeroconfRegistry::ID id>
class TZCType : public ZeroconfRegistry
{
public:
    
    /** Obtain the configured Zeroconf type for objects derived from this class.

        \return NULL-terminated C string
    */
    static const char* GetZeroconfType()
	{ return GetType(id); }

    /** Obtain the "twin" Zeroconf type for objects derived from this class. A "twin" is the counterpart of a
        publisher/subscriber pair. If the template class was defined to be a publisher, then this class method
        should return the appropriate subscriber type.

        \return NULL-terminated C string
    */
    static const char* GetTwinZeroconfType()
	{ return GetType(GetTwin(id)); }

    /** Construct a valid SideCar Zeroconf type string for a given subtype.

        \param subType the data type restriction to apply (eg. Video)

        \return type string
    */
    static std::string MakeZeroconfType(const std::string& subType)
	{ return ZeroconfRegistry::MakeZeroconfType(GetZeroconfType(),
                                                    subType); }

    /** Construct the twin SideCar Zeroconf type string for a given subtype.

        \param subType the data type restriction to apply (eg. Video)

        \return type string
    */
    static std::string MakeTwinZeroconfType(const std::string& subType)
	{ return ZeroconfRegistry::MakeZeroconfType(GetTwinZeroconfType(),
                                                    subType); }
};

/** TZCType template instantiation for publishers.
 */
using Publisher = TZCType<ZeroconfRegistry::kPublisher>;

/** TZCType template instantiation for subscribers.
 */
using Subscriber = TZCType<ZeroconfRegistry::kSubscriber>;

/** TZCType template instantiation for SideCar status collectors.
 */
using RunnerStatusCollector = TZCType<ZeroconfRegistry::kRunnerStatusCollector>;

/** TZCType template instantiation for SideCar status emitters.
 */
using RunnerStatusEmitter = TZCType<ZeroconfRegistry::kRunnerStatusEmitter>;

/** TZCType template instantiation for SideCar XML-RPC servers.
 */
using RunnerRemoteController = TZCType<ZeroconfRegistry::kRunnerRemoteController>;

/** TZCType template instantiation for SideCar state emitters.
 */
using StateEmitter = TZCType<ZeroconfRegistry::kStateEmitter>;

/** TZCType template instantiation for SideCar state collectors.
 */
using StateCollector = TZCType<ZeroconfRegistry::kStateCollector>;

} // end namespace ZeroconfTypes
} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
