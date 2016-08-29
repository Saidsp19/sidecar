#include <cassert>

#include "ace/CDR_Stream.h"
#include "ace/Guard_T.h"
#include "ace/Lock_Adapter_T.h"
#include "ace/Malloc_Allocator.h" // for ACE_New_Allocator
#include "ace/Mutex.h"
#include "ace/Singleton.h"	// for ACE_Singleton

#include "Logger/Log.h"
#include "Utils/Pool.h"

#include "MessageManager.h"
#include "Preamble.h"

using namespace SideCar;
using namespace SideCar::IO;

/** A wrapper for the Utils::Pool class which allows it to be used as an ACE memory allocator. The Utils::Pool
    class is a source of same-size memory objects with fast creation/destruction times. This class serves as the
    base class for the MessageBlockAllocatorImpl, DataBlockAllocatorImpl, and MetaDataAllocatorImpl classes
    below.
*/
class PoolAllocator : public ACE_New_Allocator
{
public:

    /** Obtainn log device for PoolAllocator objects

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor.

        \param objectSize the number of bytes take up by an object in the pool.

        \param name name of the allocator (used only for log messages)
    */
    PoolAllocator(size_t objectSize, const char* name);

    /** Destructor. Deallocates all pool chunks.
     */
    ~PoolAllocator();

    /** Obtain allocation statistics for this allocator.

        \return Utils::Pool::AllocationStats value
    */
    Utils::Pool::AllocationStats getAllocationStats() const
	{ return pool_.getAllocationStats(); }

    /** Override of ACE_New_Allocator::malloc() method. Allocates a new object from the pool.

        \param size number of bytes to allocate. This should be the same value as given to the PoolAllocator
        constructor. If not, then the default C++ allocate will be used to process the request, and not the pool
        allocator.

        \return pointer to allocated space, or NULL if error
    */
    void* malloc(size_t size);

    /** Override of ACE_New_Allocator::calloc() method. Allocates a new object from the pool, and sets all of
        the bytes in the object to a given value. Uses malloc() method to do the allocation.

        \param size number of bytes to allocate. This should be the same value as given to the PoolAllocator
        constructor. If not, then the default C++ allocate will be used to process the request, and not the pool
        allocator.

	\param init initial value to assign to all the allocated bytes

        \return pointer to allocated space, or NULL if error
    */
    void* calloc(size_t size, char init = '\0');

    void* calloc(size_t count, size_t elemSize, char init = '\0')
	{ return calloc(count * elemSize, init); }

    /** Return a previously-allocated object to the pool. Normally, does not check to see if the given pointer
	originated from the pool. To do so, invoke Utils::Pool::SetValidateRelease(true) prior to deleting an
	object.

        \param ptr pointer to release
    */
    void free(void* ptr);

private:
    Utils::Pool pool_;		///< Pool allocator that manages the memory
    ACE_Thread_Mutex mutex_;	///< Mutex protecting the pool allocator
    size_t objectSize_;		///< Size of the objects held in the pool
    std::string name_;		///< Name of the allocator
};

Logger::Log&
PoolAllocator::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("SideCar.IO.MessageManager.PoolAllocator");
    return log_;
}

PoolAllocator::PoolAllocator(size_t objectSize, const char* name)
    : ACE_New_Allocator(), pool_(objectSize, 1024), mutex_(),
      objectSize_(objectSize), name_(name)
{
    Logger::ProcLog log("PoolAllocator", Log());
    LOGDEBUG << objectSize << ' ' << name << std::endl;
}

PoolAllocator::~PoolAllocator()
{
    Logger::ProcLog log("~PoolAllocator", Log());
    LOGDEBUG << name_ << std::endl;
}

void*
PoolAllocator::malloc(size_t size)
{
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return pool_.allocate(size);
}

void*
PoolAllocator::calloc(size_t size, char init)
{
    void* ptr = this->malloc(size);
    if (ptr) ACE_OS::memset(ptr, init, size);
    return ptr;
}

void
PoolAllocator::free(void* obj)
{
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    pool_.release(obj, objectSize_);
}

/** Specialization of the PoolAllocator for ACE_Message_Block objects. There is a separate allocator for the
    ACE_Data_Block object held internally by an ACE_Message_Block. Grants friend access to an ACE_Singleton
    template class in order to allow it to create and manage a singleton instance of this class.
*/
class MessageBlockAllocatorImpl : public PoolAllocator
{
    /** Constructor.
     */
    MessageBlockAllocatorImpl()
	: PoolAllocator(sizeof(ACE_Message_Block), "MessageBlock")
	{}
    friend class ACE_Singleton<MessageBlockAllocatorImpl,
			       ACE_Recursive_Thread_Mutex>;
};

/** Define an ACE_Singleton type that will manage a singleton instance of MessageBlockAllocatorImpl class. To
    obtain the singleton object, invoke MessageBlockAllocator::instance().
*/
using MessageBlockAllocator = ACE_Singleton<MessageBlockAllocatorImpl,ACE_Recursive_Thread_Mutex>;

/** Specialization of the PoolAllocator for ACE_Data_Block objects. Grants friend access to an ACE_Singleton
    template class in order to allow it to create and manage a singleton instance of this class.
*/
class DataBlockAllocatorImpl : public PoolAllocator
{
    /** Constructor.
     */
    DataBlockAllocatorImpl() : PoolAllocator(sizeof(ACE_Data_Block), "DataBlock") {}
    friend class ACE_Singleton<DataBlockAllocatorImpl, ACE_Recursive_Thread_Mutex>;
};

/** Define an ACE_Singleton type that will manage a singleton instance of DataBlockAllocatorImpl class. To
    obtain the singleton object, invoke DataBlockAllocator::instance().
*/
using DataBlockAllocator = ACE_Singleton<DataBlockAllocatorImpl,ACE_Recursive_Thread_Mutex>;

/** Specialization of the PoolAllocator for MetaData objects. Grants friend access to an ACE_Singleton template
    class in order to allow it to create and manage a singleton instance of this class.
*/
class MetaDataAllocatorImpl : public PoolAllocator
{
public:

    /** Override of PoolAllocator method. Manually invokes the destructor for the MetaData class in order to
        reduce the boost::shared_ptr reference count.

        \param obj pointer to release
    */
    void free(void* obj);

private:
    
    /** Constructor.
     */
    MetaDataAllocatorImpl() : PoolAllocator(sizeof(MessageManager::MetaData), "MetaData") {}
    friend class ACE_Singleton<MetaDataAllocatorImpl, ACE_Recursive_Thread_Mutex>;
};

/** Define an ACE_Singleton type that will manage a singleton instance of MetaDataAllocatorImpl class. To obtain
    the singleton object, invoke MetaDataAllocator::instance().
*/
using MetaDataAllocator = ACE_Singleton<MetaDataAllocatorImpl,ACE_Recursive_Thread_Mutex>;

void
MetaDataAllocatorImpl::free(void* obj)
{
    static Logger::ProcLog log("MetaDataAllocator::free", MessageManager::Log());
    LOGDEBUG << obj << std::endl;

    // Invoke the MetaData destructor so that the boost::shared_ptr is properly destructed.
    //
    reinterpret_cast<MessageManager::MetaData*>(obj)->~MetaData();
    PoolAllocator::free(obj);
}

/** Define a locking strategy for the message blocks allocated by a MessageManager. The SideCar messaging system
    uses ACE message queues to hold ACE_Message_Block objects. This locking strategy uses a mutex to protect
    ACE_Message_Block internals when they exist in a multithreaded environment.
*/
class MessageBlockLockingStrategyImpl : public ACE_Lock_Adapter<ACE_Mutex>
{
    /** Constructor.
     */
    MessageBlockLockingStrategyImpl() : ACE_Lock_Adapter<ACE_Mutex>() {}
    friend class ACE_Singleton<MessageBlockLockingStrategyImpl, ACE_Recursive_Thread_Mutex>;
};

/** Define an ACE_Singleton type that will manage a singleton instance of MessageBlockLockingStrategyImpl class.
    To obtain the singleton object, invoke MessageBlockLockingStrategy::instance().
*/
using MessageBlockLockingStrategy = ACE_Singleton<MessageBlockLockingStrategyImpl, ACE_Recursive_Thread_Mutex>;

Logger::Log&
MessageManager::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.MessageManager");
    return log_;
}

ACE_Message_Block*
MessageManager::MakeMessageBlock(size_t size, int type)
{
    return new(MessageBlockAllocator::instance()->malloc(sizeof(ACE_Message_Block)))
	ACE_Message_Block(size,
                          type,
                          0,	// continuation
                          0,	// raw data pointer
                          0,	// allocator strategy
                          MessageBlockLockingStrategy::instance(),
                          ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                          ACE_Time_Value::zero,
                          ACE_Time_Value::max_time,
                          DataBlockAllocator::instance(),
                          MessageBlockAllocator::instance());
}

ACE_Message_Block*
MessageManager::MakeControlMessage(ControlMessage::Type type, size_t size)
{
    Logger::ProcLog log("MakeControlMessage", Log());
    LOGINFO << type << ' ' << size << std::endl;
    return MakeMessageBlock(size, kControl + int(type));
}


MessageManager::MessageManager(const Messages::Header::Ref& native)
    : data_(0), metaData_(0)
{
    static Logger::ProcLog log("MessageManager(native)", Log());
    LOGINFO << this << std::endl;
    makeMetaData(0);
    metaData_->size = native->getSize();
    metaData_->native = native;
}

MessageManager::MessageManager(ACE_Message_Block* data, const Messages::MetaTypeInfo* metaTypeInfo)
    : data_(0), metaData_(0)
{
    static Logger::ProcLog log("MessageManager(data, metaTypeInfo)", Log());
    LOGTIN << "msgType: " << data->msg_type()  << " metaTypeInfo: " << metaTypeInfo << std::endl;

    switch (data->msg_type()) {
    case kRawData:
	LOGDEBUG << "kRawData" << std::endl;

	// Raw encoded data.
	//
	if (! metaTypeInfo) {

	    LOGDEBUG << "no metaTypeInfo" << std::endl;

	    // Attempt to get the MetaTypeInfo using the indicator embedded in the binary message.
            //
	    Decoder decoder(data->duplicate());
	    if (decoder.isValid()) {
		metaTypeInfo = Messages::Header::GetMessageMetaTypeInfo(decoder);
	    }
	}

	if (! metaTypeInfo) {
	    Utils::Exception ex("raw data has no meta type");
	    log.thrower(ex);
	}
	else {
	    makeMetaData(data);
	    Decoder decoder(data->duplicate());
	    metaData_->size = data->total_length();
	    metaData_->native = metaTypeInfo->getCDRLoader()(decoder);
	}
	break;

    case kMetaData:
	LOGDEBUG << "kMetaData" << std::endl;

	// Native message object.
	//
	data_ = data;
	metaData_ = reinterpret_cast<MetaData*>(data->base());
	break;

    default:
	LOGDEBUG << "default - control message" << std::endl;

	// Should be a control message
	//
	if (! IsControlMessage(data)) {
	    Utils::Exception ex("invalid message type - ");
	    ex << data->msg_type();
	    log.thrower(ex);
	}

	data_ = data;
	break;
    }

    LOGTOUT << std::endl;
}

MessageManager::~MessageManager()
{
    static Logger::ProcLog log("~MessageManager", Log());
    LOGINFO << this << std::endl;

    // NOTE: since the MetaData object resides in the storage of an ACE_Data_Block, we don't do anything with it
    // here -- when the ACE_Data_block is destroyed, the MetaDataAllocator::free method is called which will
    // invoke ~MetaData().
    //
    if (data_) data_->release();
    metaData_ = 0;
}

void
MessageManager::makeMetaData(ACE_Message_Block* raw)
{
    data_ = new(MessageBlockAllocator::instance()->malloc(sizeof(ACE_Message_Block)))
	ACE_Message_Block(sizeof(MetaData),
                          kMetaData,
                          raw,	// continuation
                          0,	// data data pointer
                          MetaDataAllocator::instance(),
                          MessageBlockLockingStrategy::instance(),
                          ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                          ACE_Time_Value::zero,
                          ACE_Time_Value::max_time,
                          DataBlockAllocator::instance(),
                          MessageBlockAllocator::instance());

    // Use placement new to initialize raw memory pointed to by data_->base().
    //
    metaData_ = new(data_->base()) MetaData;
}

ACE_Message_Block*
MessageManager::getEncoded() const
{
    static Logger::ProcLog log("getEncoded", Log());
    LOGDEBUG << data_->cont() << std::endl;

    // Cannot generate an encoded representation without a native message.
    //
    if (! hasNative()) {
	LOGERROR << "no held message object to encode" << std::endl;
	return 0;
    }

    // We must be able to lock
    //
    assert(data_->locking_strategy());

    // !!! Unprotected cont() check !!! This may be misguided, but here is my rationale: we want to hold the
    // lock for the least amount of time, so we wait until *after* we've generated the encoded data blocks, at
    // which point we safely check for a NULL cont() value. If that is still true, then we can update it with
    // our encoded data. Otherwise, someone else beat us to it and we just forget all of the encoding we did.
    //
    if (! data_->cont()) {
	LOGDEBUG << "encoding held message object" << std::endl;

	// Create an encoder for the body of the message.
	//
	ACE_OutputCDR messageEncoder(size_t(0), ACE_CDR_BYTE_ORDER, (ACE_Allocator*)(0),
                                     DataBlockAllocator::instance(), MessageBlockAllocator::instance());

	// Write out the encoded representation.
	//
	if (! metaData_->native->write(messageEncoder).good_bit()) {
	    LOGERROR << "failed to encode object " << metaData_->native.get() << std::endl;
	    return 0;
	}

	// Now create an encoder for the message preamble that contains the size of the body.
	//
	Preamble preamble(messageEncoder.total_length());
	ACE_OutputCDR preambleEncoder(Preamble::kCDRStreamSize, ACE_CDR_BYTE_ORDER, 0, // buffer allocator
                                      DataBlockAllocator::instance(), MessageBlockAllocator::instance());
	preamble.write(preambleEncoder);

	// Make a duplicate of the encoded blocks since the encoder does not give up ownership. First, the
	// preamble block, followed by the encoded message data.
	//
	ACE_Message_Block* bytes = preambleEncoder.begin()->duplicate();
	bytes->cont(messageEncoder.begin()->duplicate());

	// !!! Protected cont() check !!! Store the encoded block chain so we don't have to do this again. If
	// someone else got here first and updated the cont() field, we just forget our encoded data.
	//
	ACE_Guard<ACE_Lock> guard(*data_->locking_strategy());
	if (! data_->cont()) {
	    data_->cont(bytes);
        }
        else {
	    bytes->release();
        }
    }

    return data_->cont()->duplicate();
}

MessageManager::AllocationStats
MessageManager::GetAllocationStats()
{
    AllocationStats stats;
    stats.messageBlocks = MessageBlockAllocator::instance()->getAllocationStats();
    stats.dataBlocks = DataBlockAllocator::instance()->getAllocationStats();
    stats.metaData = MetaDataAllocator::instance()->getAllocationStats();
    return stats;
}
