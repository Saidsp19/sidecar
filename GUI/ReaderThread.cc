#include "GUI/LogUtils.h"

#include "QtMonitor.h"
#include "MessageReader.h"
#include "ReaderThread.h"
#include "ServiceEntry.h"

using namespace SideCar::GUI;;

Logger::Log&
ReaderThread::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.ReaderThread");
    return log_;
}

ReaderThread::ReaderThread()
    : QThread(), serviceEntry_(0), messages_(), active_(0), mutex_()
{
    Logger::ProcLog log("ReaderThread", Log());
    LOGINFO << std::endl;

    // Become associated with the QThread object. From this point on, any signal connections to this object will
    // be as queued connections (except for the MessageReader object created in the run() method below)
    //
    moveToThread(this);

    // Initialize our incoming message buffers. We use two so that we can collect messages in oen while another
    // thread processes the messages in another.
    //
    messages_[0].clear();
    messages_[1].clear();
}

ReaderThread::~ReaderThread()
{
    Logger::ProcLog log("~ReaderThread", Log());
    LOGINFO << this << std::endl;
    if (isRunning())
	stop();
}

void
ReaderThread::useServiceEntry(const ServiceEntry* serviceEntry)
{
    Logger::ProcLog log("useServiceEntry", Log());
    LOGINFO << this << " serviceEntry: " << serviceEntry << std::endl;

    if (isRunning())
	stop();

    if (serviceEntry) {

	// Create a duplicate of the given ServiceEntry since we don't know its lifetime.
	//
	serviceEntry_.reset(serviceEntry->duplicate());
	start();
    }
}

void
ReaderThread::stop()
{
    Logger::ProcLog log("stop", Log());
    LOGINFO << this << std::endl;
    if (isRunning()) {
	quit();
	wait();
    }

    serviceEntry_.reset();
}

const MessageList&
ReaderThread::getMessages()
{
    // Clear the inactive buffer before we swap it in. No need to protect this since we are the only ones using
    // it right now.
    //
    size_t retired = active_;
    size_t newActive = retired ^ 1;
    messages_[newActive].clear();

    // !!! Need to protect against other thread reading active_ value. Fetch the active buffer and begin using
    // the inactive one. *** Is there a better, non-locking way?
    //
    mutex_.lock();
    active_ = newActive;
    mutex_.unlock();

    return messages_[retired];
}

void
ReaderThread::received(const Messages::Header::Ref& msg)
{
    Logger::ProcLog log("received", Log());
    LOGINFO << std::endl;

    // !!! Need to protect against changes to active_ while adding message to list.
    //
    mutex_.lock();
    bool doSend = messages_[active_].empty();
    messages_[active_].push_back(msg);
    mutex_.unlock();

    // Notify others that we have data, but only do so the FIRST time we have data. Otherwise, we run the risk
    // of flooding another thread's event queue.
    //
    if (doSend) {
	LOGDEBUG << "data available" << std::endl;
	emit dataAvailable();
    }
}

void
ReaderThread::run()
{
    Logger::ProcLog log("run", Log());
    LOGINFO << this << std::endl;

    // Create a new MessageReader object for the thread to use to read in data from the publisher described by
    // our ServiceEntry object.
    //
    boost::scoped_ptr<MessageReader> reader(
	MessageReader::Make(serviceEntry_.get()));

    // The reader belongs to our thread, so we can connect to it without queuing signals. Also, we must delete
    // it before we and our thread exits.
    //
    connect(reader.get(), SIGNAL(connected()), SIGNAL(connected()));
    connect(reader.get(), SIGNAL(received(const Messages::Header::Ref&)),
            SLOT(received(const Messages::Header::Ref&)));
    connect(reader.get(), SIGNAL(disconnected()), SIGNAL(disconnected()));

    // Run this thread's event loop so we can respond to signals from our MessageReader.
    //
    exec();

    // Don't issue any signals just because we are deleting the MessageReader objet; we are already quitting
    // anyway.
    //
    reader->disconnect(this);
    reader.reset();

    LOGINFO << this << " reader thread exiting" << std::endl;
}
