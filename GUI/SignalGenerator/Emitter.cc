#include "QtCore/QMutexLocker"

#include "GUI/LogUtils.h"
#include "GUI/Writers.h"
#include "IO/MessageManager.h"
#include "Time/TimeStamp.h"

#include "Emitter.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::SignalGenerator;

struct Pauser
{
    Emitter* obj_;
    Pauser(Emitter* obj) : obj_(obj->stop() ? obj : 0) {}
    ~Pauser() { if (obj_) obj_->start(); }
};

Logger::Log&
Emitter::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SignalGenerator.Emitter");
    return log_;
}

Emitter::Emitter()
    : writer_(0), sleepDuration_(0), messages_(), messageIndex_(0),
      running_(false), mutex_()
{
    ;
}

Emitter::~Emitter()
{
    stop();
    if (writer_) {
	writer_->stop();
	delete writer_;
    }
}

void
Emitter::setFrequency(size_t frequency)
{
    Logger::ProcLog log("setFrequency", Log());
    Time::TimeStamp dwellTime(1.0 / frequency);
    sleepDuration_ = dwellTime.getSeconds() *
	Time::TimeStamp::kMicrosPerSecond + dwellTime.getMicro();
    LOGINFO << "sleepDuration: " << sleepDuration_ << std::endl;
}

void
Emitter::clear()
{
    QMutexLocker locker(&mutex_);
    messages_.clear();
    messageIndex_ = 0;
}

void
Emitter::addMessage(const Messages::Video::Ref& msg)
{
    QMutexLocker locker(&mutex_);
    messages_.append(msg);
}

void
Emitter::setMessageList(const QList<Messages::Video::Ref>& messages)
{
    Logger::ProcLog log("setMessageList", Log());
    LOGINFO << "message count: " << messages.size() << std::endl;
    Pauser p(this);
    messages_ = messages;
    messageIndex_ = 0;
}

MessageWriter*
Emitter::setPublisherInfo(const QString& name,
                          MainWindow::ConnectionType connectionType,
                          const QString& multicastAddress)
{
    Logger::ProcLog log("setPublisherInfo", Log());
    LOGINFO << "name: " << name << " connectionType: " << connectionType
	    << " multicastAddress: " << multicastAddress << std::endl;

    Pauser p(this);

    if (writer_) {
	writer_->stop();
	delete writer_;
	writer_ = 0;
    }

    if (connectionType == MainWindow::kMulticast) 
	writer_ = UDPMessageWriter::Make(
	    name, Messages::Video::GetMetaTypeInfo().getName(),
	    multicastAddress);
    else
	writer_ = TCPMessageWriter::Make(
	    name, Messages::Video::GetMetaTypeInfo().getName());

    if (! writer_)
	LOGERROR << "failed to create new MessageWriter" << std::endl;

    return writer_;
}

bool
Emitter::start()
{
    if (running_ || ! writer_ || messages_.empty())
	return false;
    running_ = true;
    Super::start();
    return true;
}

bool
Emitter::stop()
{
    if (! running_) return false;
    running_ = false;
    wait();
    return true;
}

void
Emitter::rewind()
{
    Pauser p(this);
    messageIndex_ = 0;
}

void
Emitter::run()
{
    while (running_) {
	mutex_.lock();
	if (! messages_.empty()) {
	    IO::MessageManager mgr(messages_[messageIndex_++]);
	    if (messageIndex_ == messages_.size())
		messageIndex_ = 0;
	    mutex_.unlock();
	    writer_->writeMessage(mgr);
	}
	else {
	    mutex_.unlock();
	}
	usleep(sleepDuration_);
    }
}
