#include "Logger/Log.h"

#include "LoaderRegistry.h"

using namespace SideCar::Messages;

Logger::Log&
VoidLoaderRegistry::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Messages.VoidLoaderRegistry");
    return log_;
}

VoidLoaderRegistry::VoidLoaderRegistry(VersionType version, VoidLoader loader) : loaders_()
{
    static Logger::ProcLog log("VoidLoaderRegistry", Log());
    LOGINFO << std::endl;
    addVoidLoader(version, loader);
}

VoidLoaderRegistry::VoidLoaderRegistry(const VersionedVoidLoader* first, size_t count) :
    loaders_()
{
    static Logger::ProcLog log("VoidLoaderRegistry", Log());
    LOGINFO << count << std::endl;

    if (!count) {
        Utils::Exception ex("called with zero loaders");
        log.thrower(ex);
    }

    // We *could* have filled the vector directly in the constructor, using two iterators, and then sort it.
    // However, we want to check for duplicates, so we manually insert one at a time, keeping the vector sorted
    // as we go along.
    //
    loaders_.reserve(count);
    while (count-- > 0) {
        addVoidLoader(first->version_, first->loader_);
        ++first;
    }
}

void
VoidLoaderRegistry::dump() const
{
    static Logger::ProcLog log("dump", Log());
    LOGDEBUG << "count: " << loaders_.size() << std::endl;
    auto pos(loaders_.begin());
    auto end(loaders_.end());
    while (pos != end) {
        LOGDEBUG << pos->version_ << ' ' << ptrdiff_t(pos->loader_) << std::endl;
        ++pos;
    }
}

void
VoidLoaderRegistry::addVoidLoader(VersionType version, VoidLoader loader)
{
    static Logger::ProcLog log("addLoader", Log());
    LOGINFO << version << ' ' << ptrdiff_t(loader) << std::endl;

    LOGINFO << "*** old ***" << std::endl;
    dump();

    // Quick check to see if we can keep the order with just an add to the end of the vector.
    //
    VersionedVoidLoader newEntry(version, loader);
    if (loaders_.empty() || version > loaders_.back().version_) {
        loaders_.push_back(newEntry);
        latestLoader_ = loader;
        currentVersion_ = version;
        LOGINFO << "*** new ***" << std::endl;
        dump();
        return;
    }

    // Search for the position in the set of existing loaders where an insertion would not change the ordering
    // of the items. If an entry with the same version value already exists, the following will return an
    // iterator that is one beyond the entry so we need to check the entry prior to the returned position to see
    // if it has the same version indicator.
    //
    VersionedVoidLoaderVector::iterator pos(std::upper_bound(loaders_.begin(), loaders_.end(), newEntry));
    if (pos != loaders_.begin() && (pos - 1)->version_ == version) {
        Utils::Exception ex("loader already registered for version ");
        ex << version << " for message type ";
        log.thrower(ex);
    }

    if (pos != loaders_.end()) {
        loaders_.insert(pos, newEntry);
    } else {
        loaders_.push_back(newEntry);
        latestLoader_ = loader;
        currentVersion_ = version;
    }

    LOGDEBUG << "latest: " << latestLoader_ << " currentVersion: " << currentVersion_ << std::endl;
    LOGINFO << "*** new ***" << std::endl;

    dump();
}

VoidLoaderRegistry::VoidLoader
VoidLoaderRegistry::getVoidLoader(VersionType version) const
{
    static Logger::ProcLog log("getVoidLoader", Log());
    LOGINFO << "looking for version: " << version << std::endl;

    // This should never happen, since our constructors do not allow for an instance to exist without at least
    // one registration.
    //
    if (loaders_.empty()) {
        Utils::Exception ex("no loaders registered version ");
        ex << version;
        log.thrower(ex);
    }

    // Quick check to see if we want the latest loader.
    //
    if (version == currentVersion_) return latestLoader_;

    VersionedVoidLoaderVector::const_iterator pos(
        std::lower_bound(loaders_.begin(), loaders_.end(), VersionedVoidLoader(version, 0)));
    LOGDEBUG << "std::lower_bound offset: " << (pos - loaders_.begin()) << std::endl;
    if (pos == loaders_.end()) {
        --pos;
        LOGWARNING << "version " << version << " is greater than version of last loader " << currentVersion_
                   << std::endl;
    }

    LOGDEBUG << pos->version_ << ' ' << ptrdiff_t(pos->loader_) << std::endl;

    return pos->loader_;
}
