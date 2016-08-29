#include <unistd.h>
#include "boost/bind.hpp"

#include "Algorithms/Controller.h"
#include "Algorithms/ShutdownMonitor.h"
#include "GUI/LogUtils.h"
#include "IO/FileReaderTask.h"
#include "IO/FileWriterTask.h"
#include "IO/MulticastDataPublisher.h"
#include "IO/MulticastDataSubscriber.h"
#include "IO/MulticastVMEReaderTask.h"
#include "IO/ParametersChangeRequest.h"
#include "IO/ProcessingStateChangeRequest.h"
#include "IO/TCPDataPublisher.h"
#include "IO/TCPDataSubscriber.h"
#include "IO/TSPIReaderTask.h"
#include "IO/VMEReaderTask.h"

// !!!
// !!! On Mac OS X, dispatch_notify is a macro which causes issues.
// !!!
#ifdef darwin
#undef dispatch_notify
#endif

#include "IO/UDPSocketReaderTask.h"
#include "IO/UDPSocketWriterTask.h"
#include "XMLRPC/XmlRpcValue.h"

#include "App.h"
#include "StatusEmitter.h"
#include "StreamBuilder.h"

#include "QtCore/QTextStream"  // Needs to be after anything with boost::signal

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Runner;

static const char* const kScheduler = "scheduler";
static const char* const kThreadPriority = "priority";
static const char* const kThreaded = "threaded";

Logger::Log&
StreamBuilder::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("runner.StreamBuilder");
    return log_;
}

IO::Stream::Ref
StreamBuilder::Make(const QDomElement& stream, const StatusEmitter::Ref& statusEmitter,
                    const std::string& mcastAddress)
{
    Logger::ProcLog log("Make", Log());

    std::string name(stream.attribute("name").toStdString());
    if (! name.size()) {
	QString tmp = QString("Stream %1").arg(App::GetApp()->getStreamCount() + 1);
	name = tmp.toStdString();
    }

    StreamBuilder builder(name, statusEmitter, mcastAddress);

    QDomElement def = stream.firstChildElement();
    while (! def.isNull()) {
        QString type = def.nodeName();
	LOGDEBUG << "creating " << type.toStdString() << " task" << std::endl;

        if (type == "algorithm") {
            builder.makeAlgorithm(def);
        }
        else if (type == "filein") {
            builder.makeFileReader(def);
        }
        else if (type == "fileout") {
            builder.makeFileWriter(def);
        }
        else if (type == "publisher") {
            builder.makeDataPublisher(def);
        }
        else if (type == "subscriber") {
            builder.makeDataSubscriber(def);
        }
        else if (type == "vme") {
            builder.makeVMEReader(def);
        }
        else if (type == "tspi") {
            builder.makeTSPIReader(def);
        }
        else {
	    Utils::Exception ex("unknown module type: ");
	    ex << type.toStdString() << " -- LINE: " << def.lineNumber();
	    log.thrower(ex);
        }

        def = def.nextSiblingElement();
    }

    return builder.release();
}

IO::Stream::Ref
StreamBuilder::release()
{
    Logger::ProcLog log("release", Log());
    LOGINFO << std::endl;

    // If the stream contains a FileReaderTask that has its signalEndOfFile attribute set, add a ShutdownMonitor
    // task to detect the shutdown control message and properly shutdown the runner.
    //
    if (needShutdownMonitor_) {
	stream_->push(new ShutdownMonitorModule(stream_));
	needShutdownMonitor_ = false;
    }
    
    // The XML nodes for a stream appear in top-down fashion, but ACE::Stream pushes tasks in bottom-up fashion.
    //
    for (std::vector<IO::Module*>::reverse_iterator pos = modules_.rbegin();
         pos != modules_.rend(); ++pos) {
	if (needShutdownMonitor_) {
	    static const int kQueueSize = 32 * 1024; // 32K
	    IO::Task::Ref task = (**pos).getTask();
	    ACE_Message_Queue<ACE_MT_SYNCH>* queue = task->msg_queue();
	    if (queue) {
		queue->high_water_mark(kQueueSize);
		queue->low_water_mark(kQueueSize - 1);
	    }
	}

	stream_->push(*pos);
    }

    modules_.clear();
    return stream_;
}

void
StreamBuilder::addModule(const QDomElement& xml, IO::Module* module)
{
    Logger::ProcLog log("addModule", Log());
    LOGINFO << std::endl;
    IO::Task::Ref task(module->getTask());
    task->setTaskIndex(modules_.size());
    registerOutputs(xml, task);
    connectInputs(xml, task);
    modules_.push_back(module);
}

void
StreamBuilder::registerOutputs(const QDomElement& xml, IO::Task::Ref task)
{
    Logger::ProcLog log("registerOutputs", Log());
    LOGINFO << task->getTaskIndex() << std::endl;

    QDomElement output = xml.firstChildElement("output");
    while (! output.isNull()) {
	std::string type(output.attribute("type").toStdString());
	LOGDEBUG << "type: " << type << std::endl;
	if (! type.size()) {
	    Utils::Exception ex("no type for <output> element");
	    log.thrower(ex);
	}
	registerOutput(task, type, output.attribute("name").toStdString(),
                       output.attribute("channel").toStdString());
        output = output.nextSiblingElement("output");
    }
}

void
StreamBuilder::registerOutput(IO::Task::Ref task, const std::string& type, std::string name,
                              std::string channelName)
{
    Logger::ProcLog log("registerOutput", Log());
    size_t taskIndex = task->getTaskIndex();
    LOGINFO << taskIndex << " type: " << type << " name: " << name << " channelName: " << channelName
            << std::endl;

    if (! channelName.size()) {
	channelName = MakeDefaultChannelName(taskIndex, task->getNumOutputChannels());
	LOGDEBUG << "new channel name: " << channelName << std::endl;
    }

    if (! name.size()) name = channelName;

    ChannelMap::iterator pos = channels_.find(channelName);
    if (pos != channels_.end()) {
	Utils::Exception ex;
	ex << "channel '" << channelName << "' already exists";
	log.thrower(ex);
    }

    IO::Channel channel(name, type);
    channel.setSender(task);
    channels_.insert(ChannelMap::value_type(channelName, channel));
    task->addOutputChannel(channel);

    LOGDEBUG << "registered output channel" << std::endl;
}

void
StreamBuilder::connectInputs(const QDomElement& xml, IO::Task::Ref task)
{
    Logger::ProcLog log("connectInputs", Log());
    LOGINFO << task->getTaskIndex() << std::endl;

    QDomElement input = xml.firstChildElement("input");
    if (! input.isNull() && task->getTaskIndex() == 0) {
	Utils::Exception ex;
	ex << "the first task of a stream cannot have input definitions";
	log.thrower(ex);
    }

    while (! input.isNull()) {
	std::string type(input.attribute("type").toStdString());
	connectInput(task, type, input.attribute("name").toStdString(),
                     input.attribute("channel").toStdString());
        input = input.nextSiblingElement("input");
    }
}

void
StreamBuilder::connectInput(IO::Task::Ref task, std::string& type, std::string name, std::string channelName)
{
    Logger::ProcLog log("connectInput", Log());
    size_t taskIndex = task->getTaskIndex();
    LOGINFO << taskIndex << " type: " << type << " name: " << name << " channelName: " << channelName
            << std::endl;

    ChannelMap::iterator pos;
    if (channelName.size() == 0) {

	// Create a pseudo channel name when one is not specified, and if it matches one created by a previous
	// task.
	//
	for (int index = taskIndex - 1; index >= 0; --index) {
	    IO::Task::Ref prev(modules_[index]->getTask());
	    channelName = MakeDefaultChannelName(prev->getTaskIndex(), task->getNumInputChannels());
	    LOGDEBUG << "new channel name: " << channelName << std::endl;
	    pos = channels_.find(channelName);
	    if (pos != channels_.end())
		break;
	}
    }
    else {
	pos = channels_.find(channelName);
    }

    // If no match, report an error.
    //
    if (pos == channels_.end()) {
	Utils::Exception ex;
	ex << "unknown channel '" << channelName << "'";
	log.thrower(ex);
    }

    if (! name.size()) {
	name = channelName;
    }

    if (! type.size()) {
	type = pos->second.getTypeName();
    }

    // Verify that the output channel specification has the same type as the input one.
    //
    if (pos->second.getTypeName() != type) {
	Utils::Exception ex;
	ex << "input channel type '" << type << "' does not match output type '" << pos->second.getTypeName()
           << "'";
	log.thrower(ex);
    }

    // Add this task to the output specification's list of recipients. Add the input channel to the task.
    //
    pos->second.addRecipient(task, task->getNumInputChannels());
    IO::Channel channel(name, type);
    channel.setSender(task);
    task->addInputChannel(channel);

    LOGDEBUG << "connected input channel " << name << '/' << type << " to task " << taskIndex << std::endl;
}

long
StreamBuilder::getThreadPriority(const QString& attribute) const
{
    Logger::ProcLog log("getThreadPriority", Log());
    LOGWARNING << attribute.toStdString() << std::endl;
    
    long priority = ACE_DEFAULT_THREAD_PRIORITY;
    if (! attribute.isEmpty()) {
	bool ok = false;
	priority = attribute.toLong(&ok);

	if (! ok) {
	    Utils::Exception ex("invalid log priority value - ");
	    ex << attribute.toStdString();
	    log.thrower(ex);
	}
    }

    LOGDEBUG << "priority: " << priority << std::endl;
    return priority;
}

int
StreamBuilder::getBufferSize(const QDomElement& xml, int defaultValue) const
{
    Logger::ProcLog log("getInterfaceIndex", Log());

    int bufferSize = defaultValue;
    if (xml.hasAttribute("bufferSize")) {
	bool ok = false;
	bufferSize = xml.attribute("bufferSize").toInt(&ok);
	if (! ok || bufferSize < 0) {
	    LOGWARNING << "invalid bufferSize attribute - ignored" << std::endl;
	    bufferSize = defaultValue;
	}
    }

#ifdef linux
    if (bufferSize && defaultValue == 0) {
	LOGWARNING << "using custom bufferSize of " << bufferSize << " which may degrade Linux IO performance"
                   << std::endl;
    }
#endif

    return bufferSize;
}

uint32_t
StreamBuilder::getInterfaceIndex(const QDomElement& xml) const
{
    Logger::ProcLog log("getInterfaceIndex", Log());

    int32_t interface = 0;
    if (xml.hasAttribute("interface")) {
	std::string ifName = xml.attribute("interface").toStdString();
	if (ifName.size()) {
	    interface = ::if_nametoindex(ifName.c_str());
	    LOGWARNING << "interface '" << ifName << "' has index " << interface << std::endl;

	    // If not a valid name, try and treat as an integer.
	    //
	    if (interface == 0) {
		bool ok = false;
		interface = QString::fromStdString(ifName).toInt(&ok);
		if (! ok || interface < 0) interface = 0;
	    }
	}
	else {
	    LOGWARNING << "empty name for 'interface' attribute - ignored" << std::endl;
	}
    }

    LOGWARNING << "interface: " << interface << std::endl;
    return interface;
}
    
long
StreamBuilder::getThreadFlags(const QString& scheduler) const
{
    Logger::ProcLog log("getThreadFlags", Log());
    LOGWARNING << "scheduler: " << scheduler.toStdString() << std::endl;

    if (! scheduler.isEmpty()) {
	if (scheduler == "SCHED_INHERIT") {
	    return THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED;
	}
	else if (scheduler == "SCHED_FIFO") {
	    return THR_NEW_LWP | THR_JOINABLE | THR_EXPLICIT_SCHED | THR_SCHED_FIFO;
	}
	else if (scheduler == "SCHED_RR") {
	    return THR_NEW_LWP | THR_JOINABLE | THR_EXPLICIT_SCHED | THR_SCHED_RR;
	}
	else if (scheduler == "SCHED_OTHER") {
	    return THR_NEW_LWP | THR_JOINABLE | THR_EXPLICIT_SCHED | THR_SCHED_DEFAULT;
	}
	else {
	    Utils::Exception ex("invalid scheduler - ");
	    ex << scheduler.toStdString();
	    log.thrower(ex);
	}
    }

    return THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED;	
}

void
StreamBuilder::makeAlgorithm(const QDomElement& xml)
{
    Logger::ProcLog log("makeAlgorithm", Log());
    LOGINFO << std::endl;

    QString dll(xml.attribute("dll"));
    if (dll.isEmpty()) {
	Utils::Exception ex("no name for 'dll' element");
	log.thrower(ex);
    }

    QString name(xml.attribute("name", dll));
    if (! name.size()) {
	name = dll;
    }

    long threadFlags = getThreadFlags(xml.attribute(kScheduler));
    long threadPriority = getThreadPriority(xml.attribute(kThreadPriority));
    bool threaded = true;
    if (xml.hasAttribute(kThreaded)) {
	QString tmp = xml.attribute(kThreaded);
	if (tmp == "false" || tmp == "0")
	    threaded = false;
    }

    Algorithms::ControllerModule* module = new Algorithms::ControllerModule(stream_);
    addModule(xml, module);

    Algorithms::Controller::Ref controller = module->getTask();

    if (controller->getNumInputChannels() == 0) {
	LOGWARNING << "no input channels defined for the task " << name.toStdString() << std::endl;
    }

    if (controller->getNumOutputChannels() == 0) {
	LOGWARNING << "no output channels defined for the task " << name.toStdString() << std::endl;
    }

    controller->setXMLDefinition(xml);

    if (! controller->openAndInit(dll.toStdString(), name.toStdString(), 0, threadFlags, threadPriority,
                                  threaded)) {
	Utils::Exception ex("unable to open controller for ");
	ex << dll.toStdString();
	log.thrower(ex);
    }

    QDomElement param = xml.firstChildElement("param");
    if (! param.isNull()) {
	
	// Build up an XML-RPC <value> request that contains settings for all of the <param> elements.
	//
	QString buffer;
	QTextStream os(&buffer, QIODevice::WriteOnly);
	os << "<value><array><data>";
	do {
	    os << "<value><string>" << param.attribute("name")
	       << "</string></value><value><" << param.attribute("type")
	       << ">" << param.attribute("value")
	       << "</" << param.attribute("type") << "></value>";
	    param = param.nextSiblingElement("param");
	} while (! param.isNull());

	// Terminate the request, then post the change request to the controller.
	//
	os << "</data></array></value>";
	LOGDEBUG << buffer.toStdString() << std::endl;

	int offset = 0;
	XmlRpc::XmlRpcValue init(buffer.toStdString(), &offset);
	LOGDEBUG << init.toXml() << std::endl;
        controller->injectControlMessage(IO::ParametersChangeRequest(init, true));
    }
}

std::string
StreamBuilder::MakeDefaultChannelName(size_t taskIndex, size_t channelIndex)
{
    Logger::ProcLog log("MakeDefaultChannelName", Log());
    LOGINFO << taskIndex << ' ' << channelIndex << std::endl;
    std::ostringstream os;
    os << taskIndex << '-' << channelIndex;
    return os.str();
}

void
StreamBuilder::makeFileReader(const QDomElement& xml)
{
    Logger::ProcLog log("makeFileReader", Log());
    LOGINFO << std::endl;

    std::string type(xml.attribute("type").toStdString());
    if (! type.size()) {
	Utils::Exception ex("no type for 'filein' element");
	log.thrower(ex);
    }

    std::string path(xml.attribute("path").toStdString());
    if (! path.size()) {
	Utils::Exception ex("no path for 'filein' element");
	log.thrower(ex);
    }

    bool signalEndOfFile = xml.attribute("signalEndOfFile", "1").toShort();
    LOGDEBUG << "signalEndOfFile: " << signalEndOfFile << std::endl;

    long threadFlags = getThreadFlags(xml.attribute(kScheduler));
    long threadPriority = getThreadPriority(xml.attribute(kThreadPriority));

    IO::FileReaderTaskModule* module = new IO::FileReaderTaskModule(stream_);

    addModule(xml, module);

    IO::FileReaderTask::Ref reader = module->getTask();

    // If the reader does not define an output channel, create one for it.
    //
    if (reader->getNumOutputChannels() == 0) {
	registerOutput(reader, type, "", xml.attribute("channel").toStdString());
    }

    if (! reader->openAndInit(type, path, signalEndOfFile, threadFlags, threadPriority)) {
	Utils::Exception ex("unable to open file reader for path ");
	ex << path;
        log.thrower(ex);
    }

    if (signalEndOfFile) {
	needShutdownMonitor_ = true;
    }
}

void
StreamBuilder::makeFileWriter(const QDomElement &xml)
{
    Logger::ProcLog log("makeFileWriter", Log());
    LOGINFO << std::endl;

    std::string type(xml.attribute("type").toStdString());

    std::string path(xml.attribute("path").toStdString());
    if (! path.size()) {
	Utils::Exception ex("no path for 'fileout' element");
	log.thrower(ex);
    }

    bool acquireBasisTimeStamps = xml.attribute("acquireBasisTimeStamps", "1").toShort();
    LOGDEBUG << "acquireBasisTimeStamps: " << acquireBasisTimeStamps << std::endl;

    long threadFlags = getThreadFlags(xml.attribute(kScheduler));
    long threadPriority = getThreadPriority(xml.attribute(kThreadPriority));

    IO::FileWriterTaskModule* module = new IO::FileWriterTaskModule(stream_);
    addModule(xml, module);
    IO::FileWriterTask::Ref writer = module->getTask();

    // If the writer does not define an input channel, create one for it, and link to the previous task.
    //
    if (writer->getNumInputChannels() == 0) {
	connectInput(writer, type, "", xml.attribute("channel").toStdString());
    }

    if (! writer->openAndInit(type, path, acquireBasisTimeStamps,threadFlags, threadPriority)) {
	Utils::Exception ex("unable to open file writer with path ");
	ex << path;
	log.thrower(ex);
    }
}

void
StreamBuilder::makeDataPublisher(const QDomElement& xml)
{
    Logger::ProcLog log("makeDataPublisher", Log());
    LOGINFO << std::endl;

    std::string name(xml.attribute("name").toStdString());
    if (! name.size()) {
	Utils::Exception ex("no name for 'publisher' element");
	log.thrower(ex);
    }

    std::string type(xml.attribute("type").toStdString());
    uint32_t interface = getInterfaceIndex(xml);
    std::string transport(xml.attribute("transport").toStdString());

    if (! transport.size()) {
        transport = "multicast";
    }

    if (transport == "multicast") {
	makeMulticastDataPublisher(xml, name, type, interface);
    }
    else if (transport == "tcp") {
	makeTCPDataPublisher(xml, name, type, interface);
    }
    else if (transport == "udp") {
	makeUDPWriter(xml, name, type, interface);
    }
    else {
	Utils::Exception ex("invalid transport attribute - ");
	ex << transport;
	log.thrower(ex);
    }
}

void
StreamBuilder::makeMulticastDataPublisher(const QDomElement& xml, const std::string& name,
                                          const std::string& type, uint32_t interface)
{
    Logger::ProcLog log("makeMulticastDataPublisher", Log());
    LOGINFO << name << ' ' << type << ' ' << interface << std::endl;

    long threadFlags = getThreadFlags(xml.attribute(kScheduler));
    long threadPriority = getThreadPriority(xml.attribute(kThreadPriority));

    IO::MulticastDataPublisherModule* module = new IO::MulticastDataPublisherModule(stream_);
    addModule(xml, module);
    IO::MulticastDataPublisher::Ref publisher = module->getTask();

    if (interface) {
	publisher->setInterface(interface);
    }

    // If the publisher does not define an input channel, create one for it, and link to the previous task.
    //
    std::string realType(type);
    if (publisher->getNumInputChannels() == 0) {
	connectInput(publisher, realType, "", xml.attribute("channel").toStdString());
    }

    if (! publisher->openAndInit(realType, name, mcastAddress_, threadFlags, threadPriority)) {
	Utils::Exception ex("unable to open multicast data publisher named ");
	ex << name;
	log.thrower(ex);
    }
}

void
StreamBuilder::makeTCPDataPublisher(const QDomElement& xml, const std::string& name, const std::string& type,
                                    uint32_t interface)
{
    Logger::ProcLog log("makeTCPDataPublisher", Log());
    LOGINFO << name << ' ' << type << ' ' << interface << std::endl;

    int bufferSize = getBufferSize(xml, 0);
    long threadFlags = getThreadFlags(xml.attribute(kScheduler));
    long threadPriority = getThreadPriority(xml.attribute(kThreadPriority));

    IO::TCPDataPublisherModule* module = new IO::TCPDataPublisherModule(stream_);
    addModule(xml, module);
    IO::TCPDataPublisher::Ref publisher = module->getTask();

    if (interface) {
	publisher->setInterface(interface);
    }

    if (! name.size()) {
	Utils::Exception ex("no name for 'subscriber' element");
	log.thrower(ex);
    }

    uint16_t port = 0;
    if (xml.hasAttribute("port")) {
	bool ok;
	port = xml.attribute("port").toInt(&ok);
	if (! ok) {
	    Utils::Exception ex("invalid port for 'TCP publisher' - ");
	    ex << xml.attribute("port").toStdString();
	    log.thrower(ex);
	}
    }

    // If the publisher does not define an input channel, create one for it, and link to the previous task.
    //
    std::string realType(type);
    if (publisher->getNumInputChannels() == 0) {
	connectInput(publisher, realType, "", xml.attribute("channel").toStdString());
    }

    if (! publisher->openAndInit(realType, name, port, bufferSize, threadFlags, threadPriority)) {
	Utils::Exception ex("unable to open TCP data publisher named ");
	ex << name;
	log.thrower(ex);
    }
}

void
StreamBuilder::makeDataSubscriber(const QDomElement& xml)
{
    Logger::ProcLog log("makeDataSubscriber", Log());
    LOGINFO << std::endl;
    
    std::string name(xml.attribute("name").toStdString());
    if (! name.size()) {
	Utils::Exception ex("no name for 'subscriber' element");
	log.thrower(ex);
    }

    std::string type(xml.attribute("type").toStdString());
    if (! type.size()) {
	Utils::Exception ex("no type for 'subscriber' element");
	log.thrower(ex);
    }

    std::string transport(xml.attribute("transport").toStdString());
    if (! transport.size()) {
	transport = "multicast";
    }

    uint32_t interface = getInterfaceIndex(xml);

    if (transport == "multicast") {
	makeMulticastDataSubscriber(xml, name, type, interface);
    }
    else if (transport == "tcp") {
	makeTCPDataSubscriber(xml, name, type, interface);
    }
    else if (transport == "udp") {
	makeUDPReader(xml, name, type, interface);
    }
    else {
	Utils::Exception ex("invalid transport attribute - ");
	ex << transport;
	log.thrower(ex);
    }
}

void
StreamBuilder::makeMulticastDataSubscriber(const QDomElement& xml, const std::string& name,
                                           const std::string& type, uint32_t interface)
{
    Logger::ProcLog log("makeMulticastDataSubscriber", Log());
    LOGINFO << name << ' ' << type << ' ' << interface << std::endl;

    int bufferSize = getBufferSize(xml, 0);
    long threadFlags = getThreadFlags(xml.attribute(kScheduler));
    long threadPriority = getThreadPriority(xml.attribute(kThreadPriority));

    IO::MulticastDataSubscriberModule* module = new IO::MulticastDataSubscriberModule(stream_);
    addModule(xml, module);
    IO::MulticastDataSubscriber::Ref subscriber = module->getTask();

    // If the subscriber does not define an output channel, create one for it,
    //
    if (subscriber->getNumOutputChannels() == 0) {
	registerOutput(subscriber, type, "", xml.attribute("channel").toStdString());
    }

    if (! subscriber->openAndInit(type, name, bufferSize, interface, threadFlags, threadPriority)) {
	Utils::Exception ex("unable to subscribe to ");
	ex << name;
	log.thrower(ex);
    }
}

void
StreamBuilder::makeTCPDataSubscriber(const QDomElement& xml, const std::string& name, const std::string& type,
                                     uint32_t interface)
{
    Logger::ProcLog log("makeTCPDataSubscriber", Log());
    LOGINFO << name << ' ' << type << ' ' << interface << std::endl;

    int bufferSize = getBufferSize(xml, 0);

    IO::TCPDataSubscriberModule* module = new IO::TCPDataSubscriberModule(stream_);
    addModule(xml, module);
    IO::TCPDataSubscriber::Ref subscriber = module->getTask();

    // If the subscriber does not define an output channel, create one for it,
    //
    if (subscriber->getNumOutputChannels() == 0) {
	registerOutput(subscriber, type, "", xml.attribute("channel").toStdString());
    }

    if (! subscriber->openAndInit(type, name, bufferSize, interface)) {
	Utils::Exception ex("unable to subscribe to ");
	ex << name;
	log.thrower(ex);
    }
}

void
StreamBuilder::makeUDPReader(const QDomElement& xml, const std::string& name, const std::string& type,
                             uint32_t interface)
{
    Logger::ProcLog log("makeUDPReader", Log());
    LOGINFO << std::endl;

    bool ok = false;
    uint16_t port = xml.attribute("port").toInt(&ok);
    if (! ok) {
	Utils::Exception ex("invalid port for 'udp' transport - ");
	ex << xml.attribute("port").toStdString();
	log.thrower(ex);
    }

    size_t bufferSize = getBufferSize(xml, 256 * 1024);

    IO::UDPSocketReaderTaskModule* module = new IO::UDPSocketReaderTaskModule(stream_, bufferSize);
    addModule(xml, module);
    IO::UDPSocketReaderTask::Ref reader = module->getTask();

    std::string tmp(name);
    tmp += " UDPIn";
    reader->setTaskName(tmp);

    if (reader->getNumOutputChannels() == 0) {
	registerOutput(reader, type, "", xml.attribute("channel").toStdString());
    }

    if (! reader->openAndInit(type, port)) {
	Utils::Exception ex("unable to open UDP connection on port ");
	ex << port;
	log.thrower(ex);
    }
}

void
StreamBuilder::makeUDPWriter(const QDomElement& xml, const std::string& name, const std::string& type,
                             uint32_t interface)
{
    Logger::ProcLog log("makeUDPWriter", Log());
    LOGINFO << std::endl;

    std::string host(xml.attribute("host").toStdString());
    if (! host.size()) {
	Utils::Exception ex("no host name for 'udp' transport");
	log.thrower(ex);
    }

    bool ok = false;
    uint16_t port = xml.attribute("port").toInt(&ok);
    if (! ok) {
	Utils::Exception ex("invalid port for 'udp' transport - ");
	ex << xml.attribute("port").toStdString();
	log.thrower(ex);
    }

    IO::UDPSocketWriterTaskModule* module = new IO::UDPSocketWriterTaskModule(stream_);
    addModule(xml, module);
    IO::UDPSocketWriterTask::Ref writer = module->getTask();

    std::string tmp(name);
    tmp += " UDPOut";
    writer->setTaskName(tmp);

    // If the writer does not define an input channel, create one for it, and link to the previous task.
    //
    std::string realType(type);
    if (writer->getNumInputChannels() == 0) {
	connectInput(writer, realType, "", xml.attribute("channel").toStdString());
    }

    if (! writer->openAndInit(realType, host, port)) {
	Utils::Exception ex("unable to open UDP connection on host/port ");
	ex << host << '/' << port;
	log.thrower(ex);
    }
}

void
StreamBuilder::makeVMEReader(const QDomElement& xml)
{
    Logger::ProcLog log("makeVMEReader", Log());
    LOGINFO << std::endl;
    
    std::string host(xml.attribute("host").toStdString());
    if (! host.size()) {
	Utils::Exception ex("no host name for 'vme' element");
	log.thrower(ex);
    }

    bool ok = false;
    uint16_t port = xml.attribute("port").toInt(&ok);
    if (! ok) {
	Utils::Exception ex("invalid port for 'vme' element - ");
	ex << xml.attribute("port").toStdString();
	log.thrower(ex);
    }

    int bufferSize = getBufferSize(xml, 256 * 1024);
    long threadFlags = getThreadFlags(xml.attribute(kScheduler));
    long threadPriority = getThreadPriority(xml.attribute(kThreadPriority));

    IO::MulticastVMEReaderTaskModule* module = new IO::MulticastVMEReaderTaskModule(stream_);
    addModule(xml, module);
    IO::MulticastVMEReaderTask::Ref reader = module->getTask();

    if (reader->getNumOutputChannels() == 0) {
	registerOutput(reader, "RawVideo", "", xml.attribute("channel").toStdString());
    }

    if (! reader->openAndInit(host, port, bufferSize, threadFlags, threadPriority)) {
	Utils::Exception ex("unable to open VME connection on ");
	ex << host << '/' << port;
	log.thrower(ex);
    }
}

void
StreamBuilder::makeTSPIReader(const QDomElement& xml)
{
    Logger::ProcLog log("makeTSPIReader", Log());
    LOGINFO << std::endl;

    std::string host(xml.attribute("host").toStdString());

    bool ok = false;
    uint16_t port = xml.attribute("port").toInt(&ok);
    if (! ok) {
	Utils::Exception ex("invalid port for 'tspi' element - ");
	ex << xml.attribute("port").toStdString();
	log.thrower(ex);
    }

    int bufferSize = getBufferSize(xml, 1024 * 1024);
    long threadFlags = getThreadFlags(xml.attribute(kScheduler));
    long threadPriority = getThreadPriority(xml.attribute(kThreadPriority));

    IO::TSPIReaderTaskModule* module = new IO::TSPIReaderTaskModule(stream_);
    addModule(xml, module);
    IO::TSPIReaderTask::Ref reader = module->getTask();

    if (reader->getNumOutputChannels() == 0) {
	registerOutput(reader, "TSPI", "", xml.attribute("channel").toStdString());
    }

    if (! reader->openAndInit(host, port, bufferSize, threadFlags, threadPriority)) {
	Utils::Exception ex("unable to open TSPI connection on ");
	ex << host << '/' << port;
	log.thrower(ex);
    }
}
