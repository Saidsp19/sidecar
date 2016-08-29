#include "Logger/Log.h"

#include "CircularBuffer.h"

using namespace SideCar::Messages;

Threading::Mutex::Ref CircularBuffer::collectionMutex_ =
    Threading::Mutex::Make();
CircularBuffer::CircularBufferMap* CircularBuffer::collection_ = 0;

Logger::Log&
CircularBuffer::Log()
{
    static Logger::Log& log =
	Logger::Log::Find("SideCar.Algorithms.CircularBuffer");
    return log;
}

const CircularBuffer*
CircularBuffer::Get(const std::string& name,
                    const Messages::MetaTypeInfo& typeInfo)
{
    static Logger::ProcLog log("Get", Log());
    LOGINFO << name << ' ' << typeInfo.getKey() << std::endl;

    Threading::Locker lock(collectionMutex_);

    if (! collection_) {
	Utils::Exception ex("CircularBuffer with name '");
	ex << name << "' does not exist";
	log.thrower(ex);
    }

    CircularBufferMap::iterator pos = collection_->find(name);
    if (pos == collection_->end() ||
        pos->second->getType() != typeInfo.getKey()) {
	Utils::Exception ex("CircularBuffer with name '");
	ex << name << "' does not exist";
	log.thrower(ex);
    }

    return pos->second;
}

void
CircularBuffer::Install(const std::string& name, CircularBuffer* buffer)
{
    static Logger::ProcLog log("Install", Log());
    LOGINFO << name << ' ' << buffer << std::endl;

    Threading::Locker lock(collectionMutex_);

    if (! collection_)
	collection_ = new CircularBufferMap;

    CircularBufferMap::iterator pos = collection_->find(name);
    if (pos != collection_->end()) {
	delete buffer;
	Utils::Exception ex("CircularBuffer with name '");
	ex << name << "' already exists";
	log.thrower(ex);
    }

    (*collection_)[name] = buffer;
}

CircularBuffer::CircularBuffer(const Messages::MetaTypeInfo& typeInfo)
    : buffer_(), type_(typeInfo.getKey()), oldest_(0),
      last_(1)
{
    static Logger::ProcLog log("CircularBuffer", Log());
    LOGINFO << std::endl;
    buffer_.reserve(RadarConfig::GetShaftEncodingMax() + 1);
}

CircularBuffer::~CircularBuffer()
{
    static Logger::ProcLog log("~CircularBuffer", Log());
    LOGINFO << std::endl;
}

void
CircularBuffer::clear()
{
    static Logger::ProcLog log("clear", Log());
    LOGINFO << std::endl;
    buffer_.clear();
    oldest_ = 0;
    last_ = 1;
}

void
CircularBuffer::add(const MessageRef& msg)
{
    static Logger::ProcLog log("add", Log());
    LOGINFO << std::endl;

    if (msg->getMetaTypeInfo().getKey() != type_) {
	Utils::Exception ex("Attempt to add message type ");
	ex << msg->getMetaTypeInfo().getName() << " to buffer containing "
	   << Messages::MetaTypeInfo::Find(type_)->getName() << " messages.";
	log.thrower(ex);
    }

    size_t shaft = msg->getShaftEncoding();
    LOGDEBUG << "shaft: " << shaft << std::endl;
    if (shaft >= buffer_.size()) {
	buffer_.resize(shaft + 1);
	LOGDEBUG << "resizing buffer: " << (shaft + 1) << std::endl;
    }

    size_t index = last_ + 1;
    if (shaft < index) {
	for (; index < buffer_.size(); ++index)
	    buffer_[index].reset();
	index = 0;
    }

    for (; index < shaft; ++index)
	buffer_[index].reset();

    buffer_[index] = msg;
    last_ = index;

    oldest_ = index + 1;
    if (oldest_ == buffer_.size()) oldest_ = 0;

    LOGDEBUG << " oldest: " << oldest_ << std::endl;
}

CircularBufferIterator
CircularBuffer::oldest() const
{
    return CircularBufferIterator(this, oldest_);
}

CircularBufferIterator
CircularBuffer::newest() const
{
    size_t index = oldest_;
    if (index == 0) index = buffer_.size();
    return CircularBufferIterator(this, index - 1);
}

CircularBufferIterator
CircularBuffer::locate(const MessageRef& msg) const
{
    return CircularBufferIterator(this, msg->getShaftEncoding());
}

size_t
CircularBuffer::increment(size_t index) const
{
    static Logger::ProcLog log("increment", Log());
    LOGINFO << index << ' ' << buffer_.size() << std::endl;

    // NOTE: don't wrap at buffer_.size() since we may be increasing the number of items in the buffer until we
    // reach a preset capacity. However, to be able to obtain the zeroth element, we make sure that we handle
    // when index == buffer_.size() in the get() method.
    //
    ++index;
    if (index > buffer_.size()) index = 1;
    LOGDEBUG << index << std::endl;
    return index;
}

size_t
CircularBuffer::decrement(size_t index) const
{
    static Logger::ProcLog log("decrement", Log());
    LOGINFO << index << ' ' << buffer_.size() << std::endl;

    // Unlike the increment case, we wrap to the end of the buffer if the index is at the beginning.
    //
    if (index == 0) index = buffer_.size();
    --index;
    LOGDEBUG << index << std::endl;
    return index;
}

void
CircularBufferIterator::moveToNewer(int offset)
{
    if (offset < 0) {
	moveToOlder(-offset);
	return;
    }

    while (! atNewest() && offset-- > 0)
	index_ = buffer_->increment(index_);
}

void
CircularBufferIterator::moveToOlder(int offset)
{
    if (offset < 0) {
	moveToNewer(-offset);
	return;
    }

    while (! atOldest() && offset-- > 0)
	index_ = buffer_->decrement(index_);
}
