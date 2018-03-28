#ifndef SIDECAR_RUNNER_STREAMBUILDER_H // -*- C++ -*-
#define SIDECAR_RUNNER_STREAMBUILDER_H

#include <map>
#include <string>

#include "QtXml/QDomElement"

#include "IO/Stream.h"
#include "Runner/StatusEmitter.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {
class Module;
}
namespace Runner {

/** Utility class that creates and populates a new IO::Stream object.
 */
class StreamBuilder {
public:
    static Logger::Log& Log();

    /** Factory method that creates a new IO::Stream object

        \param config XML configuration for the stream

        \return new stream object
    */
    static IO::Stream::Ref Make(const QDomElement& config, const StatusEmitter::Ref& emitter,
                                const std::string& mcastAddress);

private:
    using ChannelMap = std::map<std::string, IO::Channel>;

    /** Constructor.

        \param name the name of the stream
    */
    StreamBuilder(const std::string& name, const StatusEmitter::Ref& emitter, const std::string& mcastAddress) :
        stream_(IO::Stream::Make(name, emitter)), modules_(), channels_(), mcastAddress_(mcastAddress),
        needShutdownMonitor_(false)
    {
    }

    /** Obtain the internal IO::Stream object. First, adds configured IO::Module objects to the stream in
        reverse order -- the ACE Stream object operates as a stack.

        \return stream object
    */
    IO::Stream::Ref release();

    /** Create a new Controller task and add to the active stream.

        \param xml configuration information for the task
    */
    void makeAlgorithm(const QDomElement& xml);

    /** Create a new FileReader task and add to the active stream.

        \param xml configuration information for the task
    */
    void makeFileReader(const QDomElement& xml);

    /** Create a new FileWriter task and add to the active stream.

        \param xml configuration information for the task
    */
    void makeFileWriter(const QDomElement& xml);

    /** Create a new DataPublisher task and add to the active stream.

        \param xml configuration information for the task
    */
    void makeDataPublisher(const QDomElement& xml);

    void makeMulticastDataPublisher(const QDomElement& xml, const std::string& name, const std::string& type,
                                    uint32_t interface);

    void makeTCPDataPublisher(const QDomElement& xml, const std::string& name, const std::string& type,
                              uint32_t interface);

    /** Create a new DataSubscriber task and add to the active stream.

        \param xml configuration information for the task
    */
    void makeDataSubscriber(const QDomElement& xml);

    void makeMulticastDataSubscriber(const QDomElement& xml, const std::string& name, const std::string& type,
                                     uint32_t interface);

    void makeTCPDataSubscriber(const QDomElement& xml, const std::string& name, const std::string& type,
                               uint32_t interface);

    /** Create a new VMEReader task and add to the active stream.

        \param xml configuration information for the task
    */
    void makeVMEReader(const QDomElement& xml);

    /** Create a new UDPSocketReaderTask task and add to the active stream.
     */
    void makeUDPReader(const QDomElement& xml, const std::string& name, const std::string& type, uint32_t interface);

    /** Create a new UDPSocketWriterTask task and add to the active stream.
     */
    void makeUDPWriter(const QDomElement& xml, const std::string& name, const std::string& type, uint32_t interface);

    /** Create a new VMEReader task and add to the active stream.

        \param xml configuration information for the task
    */
    void makeTSPIReader(const QDomElement& xml);

    /** Add a new module to the active stream. Assigns a common task ID to the Module's task, registers output
        channels, and connects input channels to previous task output channels.

        \param xml configuration for the task held by the module (containing
        any 'input' and/or 'output' elements)

        \param module the new Module to add
    */
    void addModule(const QDomElement& xml, IO::Module* module);

    /** Connect defined inputs for a task to existing outputs from previous tasks.

        \param xml configuration for the task

        \param task the task to configure
    */
    void connectInputs(const QDomElement& xml, IO::Task::Ref task);

    /** Connect one input for a task.

        \param task task to connect

        \param type channel type

        \param name name registered with the task for the channel

        \param channelName channel name to connect
    */
    void connectInput(IO::Task::Ref task, std::string& type, std::string name, std::string channelName);

    /** Create defined outputs for a task.

        \param xml configuration for the task

        \param task the task to configure
    */
    void registerOutputs(const QDomElement& xml, IO::Task::Ref task);

    /** Register one output for a task

        \param task task to register

        \param type channel type to assign

        \param name name registered with the task for the channel

        \param channelName name of channel to create
    */
    void registerOutput(IO::Task::Ref task, const std::string& type, std::string name, std::string channelName);

    uint32_t getInterfaceIndex(const QDomElement& xml) const;
    int getBufferSize(const QDomElement& xml, int defaultValue = 0) const;

    long getThreadFlags(const QString& scheduler) const;

    long getThreadPriority(const QString& attribute) const;

    /** Make a unique channel name for a channel that did not have one.

        \param taskIndex unique index assigned to the task with the channel

        \param channelIndex unique index of the channel to create

        \return new channel name
    */
    static std::string MakeDefaultChannelName(size_t taskIndex, size_t channelIndex);

    IO::Stream::Ref stream_;
    std::vector<IO::Module*> modules_;
    ChannelMap channels_;
    std::string mcastAddress_;
    bool needShutdownMonitor_;
};

} // end namespace Runner
} // end namespace SideCar

/** \file
 */

#endif
