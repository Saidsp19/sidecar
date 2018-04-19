#include <algorithm>
#include <cstring> // for tolower
#include <string>
#include <vector>

#include "ace/Guard_T.h"
#include "boost/assign/list_of.hpp"

#include "Logger/Log.h"
#include "Threading/Threading.h"
#include "Utils/Exception.h"

#include "Header.h"
#include "MetaTypeInfo.h"

using namespace SideCar::Messages;

/** Internal class that provides monotonically increasing sequence numbers for messages. There is a separate
    generator for each thread that requests a sequence number so that threads will not generate sequences with
    gaps in them.
*/
class MetaTypeInfo::SequenceGenerator {
public:
    class PerThreadInfo;

    static Logger::Log& Log()
    {
        static Logger::Log& log_ = Logger::Log::Find("SideCar.Messages.MetaTypeInfo::SequenceGenerator");
        return log_;
    }

    /** Constructor. Creates a unique pthread_key_t for a MetaTypeInfo object.
     */
    SequenceGenerator();

    /** Destructor. Destroys the unique pthread_key_t allocated by the constructor.
     */
    ~SequenceGenerator();

    /** Access the thread-specific sequence number generator and return the next sequence value from it.

        \return next sequence number
    */
    SequenceType getNextSequenceNumber();

private:
    pthread_key_t perThreadInfoKey_;
};

/** Internal message sequence counter that exists for each thread.
 */
struct MetaTypeInfo::SequenceGenerator::PerThreadInfo {
    /** Constructor. Initialize the sequence counter to 0
     */
    PerThreadInfo() : sequenceCounter_(0) {}

    /** Obtain the next sequence counter value.

        \return next sequence counter
    */
    SequenceType getNextSequenceNumber() { return ++sequenceCounter_; }

    /** Thread-specific sequence counter
     */
    SequenceType sequenceCounter_;
};

extern "C" {

/** Stub C function that deletes the PerThreadInfo object associated with a thread that is terminating.

    \param obj the PerThreadInfo object to delete
*/
static void
MetaTypeInfoDestroyPerThreadInfoStub(void* obj)
{
    using PerThreadInfo = MetaTypeInfo::SequenceGenerator::PerThreadInfo;
    delete static_cast<PerThreadInfo*>(obj);
}
}

MetaTypeInfo::SequenceGenerator::SequenceGenerator()
{
    Logger::ProcLog log("SequenceGenerator", Log());
    int rc = ::pthread_key_create(&perThreadInfoKey_, &MetaTypeInfoDestroyPerThreadInfoStub);
    if (rc) LOGERROR << "*** failed pthread_key_create: " << rc << " - " << ::strerror(rc) << std::endl;
}

MetaTypeInfo::SequenceGenerator::~SequenceGenerator()
{
    Logger::ProcLog log("SequenceGenerator", Log());
    int rc = ::pthread_key_delete(perThreadInfoKey_);
    if (rc) LOGERROR << "*** failed pthread_key_delete: " << rc << " - " << ::strerror(rc) << std::endl;
}

MetaTypeInfo::SequenceType
MetaTypeInfo::SequenceGenerator::getNextSequenceNumber()
{
    static Logger::ProcLog log("getNextSequenceNumber", Log());
    LOGINFO << std::endl;
    PerThreadInfo* pti = static_cast<PerThreadInfo*>(::pthread_getspecific(perThreadInfoKey_));
    if (!pti) {
        LOGINFO << "created new PerThreadInfo object" << std::endl;
        pti = new PerThreadInfo();
        ::pthread_setspecific(perThreadInfoKey_, pti);
    }

    return pti->getNextSequenceNumber();
}

/** Internal class that mananges MetaTypeInfo registrations.
 */
struct MetaTypeInfo::Registrations {
    using KeyVector = std::vector<int>;
    using MetaTypeInfoVector = std::vector<MetaTypeInfo*>;
    KeyVector keys_;
    MetaTypeInfoVector infos_;

    static Logger::Log& Log();

    Registrations() : keys_(), infos_() {}

    void add(int, MetaTypeInfo*);
    void remove(int);
    void dump() const;
    const MetaTypeInfo* find(int key) const;
    const MetaTypeInfo* find(const std::string& name) const;
};

Logger::Log&
MetaTypeInfo::Registrations::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Messages.MetaTypeInfo::Registrations");
    return log_;
}

void
MetaTypeInfo::Registrations::add(int key, MetaTypeInfo* info)
{
    static Logger::ProcLog log("add", Log());
    LOGDEBUG << keys_.size() << ' ' << infos_.size() << std::endl;

    if (keys_.empty() || key > keys_.back()) {
        LOGDEBUG << "adding to back" << std::endl;
        keys_.push_back(key);
        infos_.push_back(info);
    } else {
        auto pos = std::upper_bound(keys_.begin(), keys_.end(), key);
        auto index = std::distance(keys_.begin(), pos);
        if (pos != keys_.begin() && *(pos - 1) == key) {
            Utils::Exception ex("duplicate MetaTypeInfo keys - ");
            const auto& existing = infos_[index - 1];
            ex << int(key) << '/' << info->getName() << ' ' << existing->getName() << ' ' << info << ' ' << existing;
            log.thrower(ex);
        }

        LOGDEBUG << keys_.size() << ' ' << index << std::endl;
        keys_.insert(pos, key);
        infos_.insert(infos_.begin() + index, info);
    }

    dump();
}

void
MetaTypeInfo::Registrations::dump() const
{
    static Logger::ProcLog log("dump", Log());
    for (auto& info : infos_) {
        LOGDEBUG << info->getKey() << '/' << info->getName() << std::endl;
    }
}

void
MetaTypeInfo::Registrations::remove(int key)
{
    static Logger::ProcLog log("remove", Log());
    LOGDEBUG << keys_.size() << ' ' << infos_.size() << std::endl;

    KeyVector::iterator pos(std::lower_bound(keys_.begin(), keys_.end(), key));
    if (pos == keys_.end()) {
        LOGINFO << "object is not registered" << std::endl;
        return;
    }

    auto index = std::distance(keys_.begin(), pos);
    LOGDEBUG << keys_.size() << ' ' << index << std::endl;
    keys_.erase(pos);
    infos_.erase(infos_.begin() + index);
    dump();
}

const MetaTypeInfo*
MetaTypeInfo::Registrations::find(int key) const
{
    static Logger::ProcLog log("Find", Log());
    LOGINFO << key << std::endl;

    KeyVector::const_iterator pos(std::lower_bound(keys_.begin(), keys_.end(), key));
    if (pos == keys_.end()) {
        Utils::Exception ex("no MetaTypeInfo registered for key ");
        ex << int(key);
        log.thrower(ex);
    }

    return infos_[std::distance(keys_.begin(), pos)];
}

const MetaTypeInfo*
MetaTypeInfo::Registrations::find(const std::string& name) const
{
    static Logger::ProcLog log("find(name)", Log());
    LOGINFO << name << std::endl;

    MetaTypeInfoVector::const_iterator pos(infos_.begin());
    MetaTypeInfoVector::const_iterator end(infos_.end());
    while (pos != end) {
        if ((**pos).getName() == name) break;
        ++pos;
    }

    if (pos == end) {
        Utils::Exception ex("no MetaTypeInfo registered with name ");
        ex << name;
        log.thrower(ex);
    }

    return *pos;
}

ACE_Mutex MetaTypeInfo::mutex_;
MetaTypeInfo::Registrations* MetaTypeInfo::registrations_ = 0;

Logger::Log&
MetaTypeInfo::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Messages.MetaTypeInfo");
    return log_;
}

MetaTypeInfo::MetaTypeInfo(Value key, const std::string& name, CDRLoader cdrLoader, XMLLoader xmlLoader) :
    key_(key), name_(name), cdrLoader_(cdrLoader), xmlLoader_(xmlLoader), sequenceGenerator_(new SequenceGenerator)
{
    static Logger::ProcLog log("MetaTypeInfo", Log());
    LOGINFO << key << '/' << name << ' ' << std::endl;
    ACE_Guard<ACE_Mutex> locker(mutex_);
    if (!registrations_) registrations_ = new Registrations;
    registrations_->add(GetValueValue(key_), this);
}

MetaTypeInfo::~MetaTypeInfo()
{
    delete sequenceGenerator_;
}

void
MetaTypeInfo::unregister()
{
    static Logger::ProcLog log("unregister", Log());
    LOGINFO << key_ << '/' << name_ << std::endl;
    ACE_Guard<ACE_Mutex> locker(mutex_);
    registrations_->remove(GetValueValue(key_));
}

MetaTypeInfo::Value
MetaTypeInfo::getKey() const
{
    return key_;
}

const std::string&
MetaTypeInfo::getName() const
{
    return name_;
}

MetaTypeInfo::SequenceType
MetaTypeInfo::getNextSequenceNumber() const
{
    return sequenceGenerator_->getNextSequenceNumber();
}

MetaTypeInfo::CDRLoader
MetaTypeInfo::getCDRLoader() const
{
    return cdrLoader_;
}

MetaTypeInfo::XMLLoader
MetaTypeInfo::getXMLLoader() const
{
    return xmlLoader_;
}

bool
MetaTypeInfo::operator<(const MetaTypeInfo& rhs) const
{
    return key_ < rhs.key_;
}

const MetaTypeInfo*
MetaTypeInfo::Find(Value key)
{
    return registrations_->find(GetValueValue(key));
}

const MetaTypeInfo*
MetaTypeInfo::Find(const std::string& name)
{
    return registrations_->find(name);
}

bool
SideCar::Messages::operator>>(ACE_InputCDR& cdr, MetaTypeInfo::Value& value)
{
    static Logger::ProcLog log("operator>>", MetaTypeInfo::Log());
    std::underlying_type_t<MetaTypeInfo::Value> tmpInt;
    cdr >> tmpInt;
    MetaTypeInfo::Value tmp(static_cast<MetaTypeInfo::Value>(tmpInt));
    if (tmp < MetaTypeInfo::Value::kInvalid || tmp > MetaTypeInfo::Value::kUnassigned) {
        Utils::Exception ex("invalid MetaTypeInfo::Value value - ");
        ex << tmp;
        log.thrower(ex);
    }
    value = tmp;
    return cdr.good_bit();
}

bool
SideCar::Messages::operator<<(ACE_OutputCDR& cdr, MetaTypeInfo::Value value)
{
    cdr << MetaTypeInfo::GetValueValue(value);
    return cdr.good_bit();
}
