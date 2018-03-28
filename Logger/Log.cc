#include <algorithm>
#include <cstring> // for tolower
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <unistd.h> // for ::getpid

#include "boost/signals2.hpp"

#include "Utils/Exception.h"
#include "Utils/Utils.h"

#include "ClockSource.h"
#include "Log.h"
#include "Msg.h"
#include "Writers.h"

using namespace Logger;

#if 0
#define DBG(A) std::clog << A << std::endl
#else
#define DBG(A)
#endif

/** Internal container for all Log objects created in the Log::MakeObj method. When this gets deleted, it will
    delete all stored Log objects as well.
*/
namespace Logger {
class LogMap {
public:
    /** Constructor.
     */
    LogMap();

    /** Destructor. Deletes all held Log devices.
     */
    ~LogMap();

    /** Locate a registered Log device for a given name.

        \param name the fully-qualified name to look for

        \return found Log device or NULL
    */
    Log* find(const std::string& name) const;

    /** Attempt to insert a new Log device into the map.

        \param log the Log device to insert

        \return true if successful
    */
    bool add(Log* log);

    std::vector<std::string> getNames();

    std::vector<std::string> getNames(Log::NewLogNotifier notifier);

private:
    using Container = std::map<std::string, Log*>;

    Container map_;
    Threading::Mutex::Ref lock_;
    boost::signals2::signal<void(Log&)> newLogSignal_;
};

} // end namespace Logger

LogMap::LogMap() : map_(), lock_(Threading::Mutex::Make()), newLogSignal_()
{
    ;
}

LogMap::~LogMap()
{
    Threading::Locker lock(lock_);
    std::for_each(map_.begin(), map_.end(), [](auto& v) { delete v.second; });
}

Log*
LogMap::find(const std::string& name) const
{
    Threading::Locker lock(lock_);
    Container::const_iterator pos = map_.find(name);
    return pos == map_.end() ? nullptr : pos->second;
}

bool
LogMap::add(Log* log)
{
    bool ok;
    {
        Threading::Locker lock(lock_);
        ok = map_.insert(Container::value_type(log->fullName(), log)).second;
        if (ok) { log->initialize(); }
    }

    if (ok && !log->isHidden()) { newLogSignal_(*log); }

    return ok;
}

std::vector<std::string>
LogMap::getNames()
{
    std::vector<std::string> names;
    Threading::Locker lock(lock_);
    std::for_each(map_.begin(), map_.end(), [&names](auto& v) {
        if (!v.second->isHidden()) names.push_back(v.first);
    });
    return names;
}

std::vector<std::string>
LogMap::getNames(Log::NewLogNotifier notifier)
{
    std::vector<std::string> names;
    Threading::Locker lock(lock_);
    newLogSignal_.connect(notifier);
    std::for_each(map_.begin(), map_.end(), [&names](auto& v) {
        if (!v.second->isHidden()) names.push_back(v.first);
    });
    return names;
}

/** Internal collection of Log attributes associated with a specific thread. To keep threads from stepping on
    each other as they build a log message, each thread has its own private LogStreamBuf object.
*/
struct PerThreadInfo {
    /** Constructor. Create a new LogStreamBuf, and a new C++ output stream object that uses it.
     */
    PerThreadInfo();

    /** Obtain a reference to the C++ output stream.

        \return std::ostream reference
    */
    std::ostream& getStream(Log* log, Priority::Level level);

    /** Obtain a reference to the C++ output stream that never collects any data due to its badbit being set.

        \return std::ostream reference
    */
    std::ostream& getNullStream() const { return *null_; }

private:
    std::unique_ptr<LogStreamBuf> lsb_;
    std::unique_ptr<std::ostream> os_;
    std::unique_ptr<std::ostream> null_;
};

PerThreadInfo::PerThreadInfo() :
    lsb_(new LogStreamBuf()), os_(new std::ostream(lsb_.get())), null_(new std::ostream(new NullLogStreamBuf))
{
    null_->setstate(std::ios_base::badbit);
}

std::ostream&
PerThreadInfo::getStream(Log* log, Priority::Level level)
{
    lsb_->setInfo(log, level);
    return *os_;
}

/** Internal collection of attributes that must be initialized only once at startup. Contains the global Log
    device map, and the global ClockSource that provides the timestamps for log messages. There is only one
    RuntimeData object in existence during the application's lifetime.
*/
struct RuntimeData {
    /** Class method that creates the RuntimeData singleton. Invoked from pthread_once() to guarantee only one
        RuntimeData object. Note that there is no way to delete the singleton value.
    */
    static void Initialize();

    /** Obtain the RuntimeData singleton value. Uses pthread_once to guarantee only one instance is created,
        regardless of threading.

        \return RuntimeData reference
    */
    static RuntimeData& Singleton();

    /** Obtain the global Log device map container.

        \return std::map reference
    */
    LogMap& logMap();

    /** Obtain the output stream object used for log messages with a priority level such that the log message
        written to the stream will NOT be written out to an output device.

        \return C++ output stream reference.
    */
    std::ostream& getNullStream();

    /** Obtain the output stream object use for log messages with a priority level such that the log message
        writtine to the stream will be written out to an output device.

        \return C++ output stream reference.
    */
    std::ostream& getStream(Log* log, Priority::Level level);

    /** Obtain the global ClockSource object for message timestamps.

        \return ClockSource object
    */
    ClockSource::Ref clock();

    /** Set the global ClockSource object for message timestamps.

        \param obj new ClockSource object to use

        \return old object value
    */
    ClockSource::Ref setClock(const ClockSource::Ref& obj);

private:
    /** Constructor. Restricted to use by Initialize() method.
     */
    RuntimeData();

    /** Destructor.
     */
    ~RuntimeData();

    LogMap logMap_;
    ClockSource::Ref clock_;
    pthread_key_t perThreadInfoKey_;

    static RuntimeData* singleton_;
    static pthread_once_t onceControl_;
};

RuntimeData* RuntimeData::singleton_ = 0;
pthread_once_t RuntimeData::onceControl_ = PTHREAD_ONCE_INIT;

extern "C" {
static void
InitializeStub()
{
    RuntimeData::Initialize();
}
static void
DestroyPerThreadInfoStub(void* obj)
{
    delete static_cast<PerThreadInfo*>(obj);
}
}

void
RuntimeData::Initialize()
{
    // Unlink ourselves (in particular std::clog) from the C std IO library.
    //
    std::ios::sync_with_stdio(false);
    singleton_ = new RuntimeData;
}

RuntimeData&
RuntimeData::Singleton()
{
    pthread_once(&onceControl_, &InitializeStub);
    return *singleton_;
}

RuntimeData::RuntimeData() : logMap_(), clock_(SystemClockSource::Make()), perThreadInfoKey_()
{
    int rc = pthread_key_create(&perThreadInfoKey_, &DestroyPerThreadInfoStub);
    if (rc) {
        std::cerr << "*** RuntimeData::RuntimeData: failed pthread_key_create: " << rc << " - " << ::strerror(rc)
                  << std::endl;
    }
}

RuntimeData::~RuntimeData()
{
    pthread_key_delete(perThreadInfoKey_);
}

LogMap&
RuntimeData::logMap()
{
    return logMap_;
}

std::ostream&
RuntimeData::getNullStream()
{
    PerThreadInfo* pti = static_cast<PerThreadInfo*>(pthread_getspecific(perThreadInfoKey_));
    if (!pti) {
        pti = new PerThreadInfo();
        pthread_setspecific(perThreadInfoKey_, pti);
    }

    return pti->getNullStream();
}

std::ostream&
RuntimeData::getStream(Log* log, Priority::Level level)
{
    if (!log->isAccepting(level)) { return getNullStream(); }

    DBG("getStream() - log: " << log->fullName() << " level: " << level);

    PerThreadInfo* pti = static_cast<PerThreadInfo*>(pthread_getspecific(perThreadInfoKey_));
    if (!pti) {
        pti = new PerThreadInfo();
        pthread_setspecific(perThreadInfoKey_, pti);
    }

    DBG("getStream() - pti: " << pti);

    return pti->getStream(log, level);
}

ClockSource::Ref
RuntimeData::clock()
{
    return clock_;
}

ClockSource::Ref
RuntimeData::setClock(const ClockSource::Ref& clock)
{
    ClockSource::Ref old = clock_;
    clock_ = clock;
    return old;
}

ClockSource::Ref
Log::SetClockSource(const ClockSource::Ref& clock)
{
    return RuntimeData::Singleton().setClock(clock);
}

ClockSource::Ref
Log::GetClockSource()
{
    return RuntimeData::Singleton().clock();
}

Log&
Log::Root()
{
    return *FindObj("root", false);
}

Log&
Log::Find(const std::string& name, bool hidden)
{
    return *FindObj(name, hidden);
}

std::vector<std::string>
Log::GetNames()
{
    return RuntimeData::Singleton().logMap().getNames();
}

std::vector<std::string>
Log::GetNames(NewLogNotifier notifier)
{
    return RuntimeData::Singleton().logMap().getNames(notifier);
}

Log*
Log::MakeObj(const std::string& name, const std::string& fullName, Log* parent, bool hidden)
{
    LogMap& map(RuntimeData::Singleton().logMap());

    // Search for the Log object in the map. If found, just return.
    //
    std::unique_ptr<Log> obj(map.find(fullName));
    if (obj) return obj.release();

    // Create new Log object and install in map.
    //
    if (parent) {
        obj.reset(new Log(name, fullName, parent, Priority::kError, hidden));
    } else {
        obj.reset(new Log("root", "root", nullptr, Priority::kError, true));
        obj->addWriter(Writers::Stream::Make(Formatters::Verbose::Make(), std::cerr, true));
    }

    if (!map.add(obj.get())) {
        // Assume that in a mad race with another thread to add a new object, that we lost. Try the find again.
        // If *that* fails then we throw an exception.
        //
        obj.reset(map.find(fullName));
        if (!obj) {
            Utils::Exception ex("Log::FetchObj: failed to insert new Log "
                                "object for key: ");
            ex << fullName;
            std::cerr << ex.err() << std::endl;
            throw ex;
        }
    }

    return obj.release();
}

std::string
Log::MakeFullName(const std::string& prefix, const std::string& tail)
{
    std::string result(prefix);
    result += ".";
    result += tail;
    return result;
}

Log*
Log::FindObj(const std::string& name, bool hidden)
{
    std::string fullName("");
    std::string objName(name);
    if (objName.size() == 0) objName = "root";
    Log* parent = nullptr;

    // See if we are trying to create the top-level object. Look for some aliases.
    //
    if (objName == "root") {
        fullName = "root";
    } else {
        // Non-root object name. See if we have a full-path. If not, use `root' as the parent.
        //
        std::string::size_type lastPeriod(objName.rfind('.'));
        if (lastPeriod > objName.size()) {
            parent = FindObj("root", false);
        } else {
            fullName = objName.substr(0, lastPeriod);
            if (fullName.size() == 0) { fullName = "root"; }

            objName = objName.substr(lastPeriod + 1, std::string::npos);
            if (objName.size() == 0) {
                std::cerr << "invalid parent name" << std::endl;
                throw std::invalid_argument(name);
            }

            parent = FindObj(fullName, false);
        }

        assert(parent && parent->name_.size());
        fullName = MakeFullName(parent->fullName(), objName);
    }

    // Look for the requested object. If not found, create it.
    //
    return MakeObj(objName, fullName, parent, hidden);
}

Log::Log(const std::string& name, const std::string& fullName, Log* parent, Priority::Level priority, bool hidden) :
    priorityLimit_(priority), maxPriorityLimit_(priority), parent_(parent), name_(name), fullName_(fullName),
    propagate_(false), hidden_(hidden), writers_(), modifyMutex_(Threading::Mutex::Make()), children_()
{
    if (parent) { maxPriorityLimit_ = std::max(priority, parent->getMaxPriorityLimit()); }
}

Log::~Log()
{
    removeAllWriters();
    if (parent_) {
        Threading::Locker lock(parent_->modifyMutex_);
        std::vector<Log*>::iterator pos = std::find(parent_->children_.begin(), parent_->children_.end(), this);
        if (pos != parent_->children_.end()) { parent_->children_.erase(pos); }
    }
}

void
Log::initialize()
{
    if (parent_) {
        Threading::Locker lock(parent_->modifyMutex_);
        parent_->children_.push_back(this);
    }
}

void
Log::setPriorityLimit(Priority::Level priority)
{
    // Record the new setting and update our maxPriorityLimit_ value.
    //
    priorityLimit_ = priority;
    if (parent_) { priority = parent_->getMaxPriorityLimit(); }
    updateMaxPriorityLimit(priority);
}

void
Log::updateMaxPriorityLimit(Priority::Level maxPriority)
{
    // Caclculate new maxPriorityLimit_ value.
    //
    maxPriority = std::max(priorityLimit_, maxPriority);
    if (maxPriority != maxPriorityLimit_) {
        // Record new setting, and force our children to recalculate their
        // maxPriorityLimit_ value.
        //
        maxPriorityLimit_ = maxPriority;
        if (!children_.empty()) {
            std::for_each(children_.begin(), children_.end(),
                          [maxPriority](auto v) { v->updateMaxPriorityLimit(maxPriority); });
        }
    }
}

void
Log::flushWriters()
{
    Threading::Locker lock(modifyMutex_);
    std::for_each(writers_.begin(), writers_.end(), [](auto v) { v->flush(); });
}

void
Log::addWriter(const Writers::Writer::Ref& writer)
{
    DBG("addWriter - log: " << this << " name: " << fullName_);
    Threading::Locker lock(modifyMutex_);
    writers_.insert(writer);
    DBG("addWriter - count: " << writers_.size());
}

void
Log::removeWriter(const Writers::Writer::Ref& writer)
{
    Threading::Locker lock(modifyMutex_);
    writers_.erase(writer);
}

void
Log::removeAllWriters()
{
    Threading::Locker lock(modifyMutex_);
    writers_.clear();
}

int
Log::numWriters() const
{
    Threading::Locker lock(modifyMutex_);
    return writers_.size();
}

std::ostream&
Log::getStream(Priority::Level level)
{
    return RuntimeData::Singleton().getStream(this, level);
}

void
Log::post(Priority::Level level, const std::string& msg) const
{
    DBG("post() - level: " << level << " msg: '" << msg << "'");
    Msg m(fullName_, msg, level);
    GetClockSource()->now(m.when_);
    const Log* p = this;
    while (p) {
        DBG("checking - p: " << p << " name: " << p->fullName_);
        if (!p->writers_.empty()) {
            DBG("post() - posting: " << p);
            p->postMsg(m);
            if (!p->propagate_) {
                DBG("no propagation - stopping");
                return;
            }
        }
        p = p->parent_;
    }
}

void
Log::postMsg(const Msg& msg) const
{
    DBG("postMsg() - writers: " << writers_.size());
    Threading::Locker lock(modifyMutex_);
    std::for_each(writers_.begin(), writers_.end(), [msg](auto v) { v->write(msg); });
}

ProcLog::ProcLog(const char* name, Log& log) : log_(Log::Find(Log::MakeFullName(log.fullName(), name), false))
{
    ;
}

ProcLog::ProcLog(const std::string& name, Log& log) : log_(Log::Find(Log::MakeFullName(log.fullName(), name), false))
{
    ;
}
