#ifndef SIDECAR_MESSAGES_LOADERREGISTRY_H // -*- C++ -*-
#define SIDECAR_MESSAGES_LOADERREGISTRY_H

#include <vector>

#include "ace/CDR_Stream.h"
#include "boost/shared_ptr.hpp"

#include "Utils/Exception.h"

namespace Logger {
class Log;
}

class ACE_InputCDR;

namespace SideCar {
namespace Messages {

/** Registry of methods that perform version-specific loads of raw data. A load procedure is a non-member
    function (eg. class method or plain C/C++ function pointer) with the following prototype:

    <code>
    ACE_InputCDR& PROC(void* OBJ, ACE_InputCDR& CDR)
    </code>

    where the PROC is the name of the function, OBJ is a pointer to an instance of a class that will be loaded,
    and CDR is a writable reference to an ACE_InputCDR stream class that handles unmarshalling of data from an
    external data source. When called, the function should fill in the object with values from the input stream,
    and it should return a reference to the stream when it is done.

    The LoaderRegistry::getLoader() method returns the appropriate loader for a particular version indicator. If
    a specific version is not found, the routine will return the loader with the closest version indicator
    greater than that requested. If there is none, the routine issues a warning message and returns the last
    registered loader Note that a class load method using the LoaderRegistry should never have this happen if
    the corresponding write method always writes out the version indicator obtained from
    LoaderRegistry::getCurrentVersion().

    Note that the class serves as a base class for the TLoaderRegistry template class below; constructor access
    restrictions prohibit direct instantiations.
*/
class VoidLoaderRegistry {
public:
    /** Type specification for loader procedures. Note that type-specific loaders exist and are specified in the
        TLoaderRegistry class.
    */
    using VoidLoader = ACE_InputCDR& (*)(void* object, ACE_InputCDR& cdr);

    /** Type specification for version indicators.
     */
    using VersionType = uint16_t;

    /** Obtain log device for instances of this class

        \return log device
    */
    static Logger::Log& Log();

    /** Obtain the latest version registered.

        \return version indicator
    */
    VersionType getCurrentVersion() const { return currentVersion_; }

protected:
    /** Pairing of specific version value with a class loader procedure. Provides ordering by version value so
        that a collection of VersionedLoader objects may be sorted.
    */
    struct VersionedVoidLoader {
        VersionedVoidLoader(VersionType version, VoidLoader loader) : version_(version), loader_(loader) {}
        bool operator<(const VersionedVoidLoader& rhs) const { return version_ < rhs.version_; }
        VersionType version_;
        VoidLoader loader_;
    };

    /** Constructor. Registers a single loader.

        \param version version indicator to register

        \param loader procedure to register
    */
    VoidLoaderRegistry(VersionType version, VoidLoader loader);

    /** Constructor. Registers an array of loaders and their versions. NOTE: throws an exception if numVersions
        is zero.

        \param first pointer to the first entry to register

        \param numVersions number of entries to register
    */
    VoidLoaderRegistry(const VersionedVoidLoader* first, size_t numVersions);

    /** Register a new version/loader combination. NOTE: if the version is already registered, the routine
        thruws an exception.

        \param version version indicator to register

        \param loader procedure to register
    */
    void addVoidLoader(VersionType version, VoidLoader loader);

    /** Obtain the appropriate loader for a given version. Note that if the version indicator is greater than
        the last registered version indicator, the routine will return the last loader. Throws an exception if
        the collection of loaders is empty.

        \param version version indicator to look for

        \return found Loader method
    */
    VoidLoader getVoidLoader(VersionType version) const;

private:
    /** Utility that emits information about current registrations
     */
    void dump() const;

    /** Container for registrations. The addLoader() method keeps it sorted by version indicator. The
        getLoader() method uses a binary search to find version indicator values.
    */
    using VersionedVoidLoaderVector = std::vector<VersionedVoidLoader>;
    VersionedVoidLoaderVector loaders_;
    VersionType currentVersion_; ///< Cached current version indicator
    VoidLoader latestLoader_;    ///< Cached latest loader procedure
};

/** Template version of the LoaderRegistry class. Provides type-safe registration and retrieval of loader
    procedures. The template parameter is a commonly the name of a class that contains methods that read from
    and write to an ACE CDR stream. For example, class definition for Foo

    <code>
    struct Foo {
      ACE_InputCDR& load(ACE_InputCDR& cdr)
        { return loaderRegistry_.load(this, cdr); }
      ACE_OutputCDR& write(ACE_OutputCDR& cdr) const
        { cdr << loaderRegistry_.getCurrentVersion(); return cdr; }
    private:
      static ACE_InputCDR& LoadV1(Foo* obj, ACE_InputCDR& cdr);
      static TLoaderRegistry<Foo>::VersionedLoader* DefineLoaders();
      static TLoaderRegistry<Foo> loaderRegistry_;
    };
    </code>

    defines a load() and a write() method to read/write instance data (here, Foo has no instance data). The
    write() method first deposits the latest version number to the CDR stream. Instance attributes would then
    follow. In the load() method, the responsibility is immediately transfered to the TLoaderRegistry::load()
    method, which expects a version indicator as the next data element in the ACE CDR input stream. The method
    then looks for an appropriate loader to call and invokes it. The definition of the loaders occurs in the
    source file (as is the case for most class members) like so:

    <code>
    TLoaderRegistry<Foo>::VersionedLoader* Foo::DefineLoaders()
    {
      static TLoaderRegistry<Foo>::VersionedLoader loaders_[] = {
        TLoaderRegistry<Foo>::VersionedLoader(1, &Foo::LoadV1)
      };
      return loaders_;
    }

    TLoaderRegistry<Foo> PRIMessageInfo::loaderRegistry_(PRIMessageInfo::DefineLoaders(), 1);
    </code>

    The DefineLoaders() class method creates an array of VersionedLoader objects, which are just pairings of
    version indicators with method pointers. Next, the loaderRegistry_ class attribute is defined, using the
    result from the DefineLoaders() class method.

    NOTE: playing with C++ class attributes may lead to fire. In particular, there are some narly initialization
    issues/rules that may cause problems. A good rule-of-thumb is to *never* refer to a class attribute from an
    inline method. Furthermore, in order for the LoaderRegistry to have a registration installed before the
    someone calls getLoader(), for all code paths that lead to a getLoader() call, there must be a call to a
    method defined and invoked in the source file where the class attribute is defined. An easy way to meet this
    requirement is to have no inlined constructor definitions, since an object must be constructed and available
    before a loader is requested in the TLoaderRegistry::load() method.
*/
template <typename T>
class TLoaderRegistry : public VoidLoaderRegistry {
public:
    using Super = VoidLoaderRegistry;

    /** Type-specific definition of the LoaderRegistry::Loader procedure specification .
     */
    using TLoader = ACE_InputCDR& (*)(T* object, ACE_InputCDR& cdr);

    /** Type-specific definition of the LoaderRegistry::VersionedLoader class.
     */
    struct VersionedLoader : public VersionedVoidLoader {
        VersionedLoader(VersionType version, TLoader loader) :
            VersionedVoidLoader(version, reinterpret_cast<VoidLoader>(loader))
        {
        }
    };

    using VersionedLoaderVector = std::vector<VersionedLoader>;

    /** Constructor. Registers a single loader.

        \param version version indicator to register

        \param loader procedure to register
    */
    TLoaderRegistry(VersionType version, TLoader loader) : Super(version, loader) {}

    /** Constructor. Registers an array of loaders and their versions.

        \param first pointer to the first entry to register

        \param numVersions number of entries to register
    */
    TLoaderRegistry(const VersionedLoader* first, size_t numVersions) : Super(first, numVersions) {}

    TLoaderRegistry(const VersionedLoaderVector& vector) : Super(&vector[0], vector.size()) {}

    /** Register a new version/loader combination. NOTE: if the version is already registered, the routine
        thruws an exception.

        \param version version indicator to register

        \param loader procedure to register
    */
    void addLoader(VersionType version, TLoader loader) { addVoidLoader(version, loader); }

    /** Find and invoke the appropriate loader procedure depending on the version indicator read in from an ACE
        CDR stream.

        \param obj the class instance to load

        \param cdr the input CDR stream containing the raw data

        \return input CDR stream reference
    */
    ACE_InputCDR& load(T* obj, ACE_InputCDR& cdr) const
    {
        VersionType version;
        cdr >> version;
        return load(obj, cdr, version);
    }

    ACE_InputCDR& load(T* obj, ACE_InputCDR& cdr, VersionType version) const { return (*getLoader(version))(obj, cdr); }

private:
    /** Type-safe reimplementation of the LoaderRegistry::getLoader() method.

        \param version version indicator to look for

        \return found Loader method
    */
    TLoader getLoader(VersionType version) const { return reinterpret_cast<TLoader>(getVoidLoader(version)); }
};

} // end namespace Messages
} // end namespace SideCar

/** \file
 */

#endif
