#include <sstream>

#include "Logger/Log.h"
#include "Publisher.h"

using namespace SideCar::Zeroconf;

Logger::Log&
Publisher::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Zeroconf.Publisher");
    return log_;
}

Publisher::Ref
Publisher::Make(Monitor* monitor)
{
    Ref ref(new Publisher(monitor));
    ref->setSelf(ref);
    return ref;
}

Publisher::Publisher(Monitor* monitor) :
    Super(monitor), publishedSignal_(), textData_(), name_(""), type_(""), domain_(""), interface_(0), port_(0),
    isPublished_(false)
{
    Logger::ProcLog log("Publisher", Log());
    LOGINFO << std::endl;
}

bool
Publisher::setTextData(const std::string& key, const std::string& value, bool postNow)
{
    if ((key.size() + value.size() + 1) > 255) return false;

    Logger::ProcLog log("Publisher", Log());
    LOGINFO << "Adding to TXTRecord " << key << "=" << value << std::endl;

    textData_[key] = value;
    return postNow ? postTextData() : true;
}

bool
Publisher::postTextData()
{
    return isRunning() ? updateTextRecord() : true;
}

/** Functor used to accumulate key/value text strings into a valid Zeroconf byte array for a TXT record. In
    short, each entry starts with a byte count describing the number of bytes that make up the entry, followed
    by the key text, an equals sign ('='), and then the value text.
*/
struct BuildBuffer {
    std::string buffer_; ///< Accumulation buffer

    /** Constructor.
     */
    BuildBuffer() : buffer_("") {}

    /** Functor method. Accumulates the given entry to the internal buffer.

        \param v entry to add
    */
    void operator()(const Publisher::StringMap::value_type& v)
    {
        // Size of each entry is the key and value sizes plus an '=' sign.
        //
        unsigned char size = v.first.size() + v.second.size() + 1;
        buffer_ += size;
        buffer_ += v.first;
        buffer_ += '=';
        buffer_ += v.second;
    }

    /** Conversion operator. Returns the contents of the internal buffer.

        \return std::string copy
    */
    operator std::string() { return buffer_; }
};

bool
Publisher::publish(const std::string& name, bool noRename)
{
    Logger::ProcLog log("publish", Log());
    LOGINFO << "Registering " << name << "." << type_ << " " << domain_ << " " << port_ << ' ' << noRename << std::endl;

    if (isRunning()) stop();

    std::string buffer(std::for_each(textData_.begin(), textData_.end(), BuildBuffer()));
    LOGDEBUG << "buffer: " << buffer << std::endl;

    // Loop here to support renaming in case of name conflict.
    //
    int counter = 1;
    name_ = name;
    while (1) {
        auto err =
            ::DNSServiceRegister(&getDNSServiceRef(), noRename ? kDNSServiceFlagsNoAutoRename : 0, interface_,
                                 name_.size() ? name_.c_str() : 0, type_.c_str(), domain_.size() ? domain_.c_str() : 0,
                                 0, htons(port_), buffer.size(), buffer.c_str(), &Publisher::RegisterCallback, this);

        if (err == kDNSServiceErr_NoError) {
            break;
        }

        // Have an error, or a name conflict and we don't want to rename.
        //
        else if (err != kDNSServiceErr_NameConflict || noRename) {
            LOGERROR << name_ << " failed DNSServiceRegister - error: " << err << std::endl;
            return false;
        }

        // Must have a name conflict. Append an increasing counter value to our original name and try again.
        //
        std::ostringstream os;
        os << name << " (" << ++counter << ")";
        name_ = os.str();
        LOGWARNING << "renaming to " << name_ << std::endl;
    }

    // Must have succeeded in starting the publishing process.
    //
    serviceStarted();
    return true;
}

void
Publisher::processResponse(DNSServiceErrorType err, const char* name, const char* type, const char* domain)
{
    Logger::ProcLog log("processResponse", Log());

    isPublished_ = err == kDNSServiceErr_NoError;

    // Update our values to reflect those used by the DNSSD server.
    //
    if (isPublished_) {
        if (name_ != std::string(name)) {
            LOGWARNING << "name conflict - new name is " << name << std::endl;
            name_ = name;
        }

        type_ = type;
        domain_ = domain;
    }

    notifyObservers(isPublished_);
}

void
Publisher::notifyObservers(bool state)
{
    publishedSignal_(state);
}

bool
Publisher::updateTextRecord()
{
    if (!isPublished_) return false;
    std::string buffer(std::for_each(textData_.begin(), textData_.end(), BuildBuffer()));
    return ::DNSServiceUpdateRecord(getDNSServiceRef(), 0, 0, buffer.size(), buffer.c_str(), 0) ==
           kDNSServiceErr_NoError;
}

void
Publisher::serviceStopping()
{
    isPublished_ = false;
    Super::serviceStopping();
}

void
Publisher::RegisterCallback(DNSServiceRef ref, DNSServiceFlags flags, DNSServiceErrorType err, const char* name,
                            const char* type, const char* domain, void* context)
{
    reinterpret_cast<Publisher*>(context)->processResponse(err, name, type, domain);
}
