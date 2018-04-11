#ifndef SIDECAR_ALGORITHMS_ALGORITHM_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_ALGORITHM_H

#include <vector>

#include "ace/svc_export.h" // for ACE_Svc_Export definition
#include <vsip/initfin.hpp>

#include "Algorithms/Controller.h"
#include "Algorithms/Processor.h"
#include "Messages/Header.h"
#include "Messages/MetaTypeInfo.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace Algorithms {

/** Base class for all algorithm classes. An algorithm exists on the system as a dynamically-loaded library
    (DLL). This abstraction allows the algorithms to run in a variety of different contexts, without the
    algorithm knowing or caring which one is in use.

    <h2>Initialization</h2>

    At a minimum, derived classes must define one or more process methods that accept a shared reference to a
    message object to perform the work defined by the algorithm. These process methods return a boolean value
    that indicates success (true) or failure (false). An algorithm may have more than one processor depending on
    the work it is doing and its data dependencies. Registration of the processors (using the
    registerProcessor() methods) should occur in the startup() method:

    \code
    bool
    Foo::startup()
    {
        registerProcessor<Foo,Messages::Video>("main", &NOOP::processMain);
        registerProcessor<Foo,Messages::Video>("aux", &NOOP::processAux);
        return true;
    }
    \endcode

    Algorithms that provide runtime parameters (see SideCar::Parameter) must also register them with the SideCar
    infrastructure so that they may respond to change requests via external XML-RPC messages. Again, this
    registration should be done in the startup() method:

    \code
    bool
    Foo::startup()
    {
        registerProcessor<Foo,Messages::Video>("main", &NOOP::processMain);
        registerProcessor<Foo,Messages::Video>("aux", &NOOP::processAux);
        return registerParameter(param1_) && registerParameter(param2_);
    }
    \endcode

    NOTE: the processor registration methods throw an exception whenever an error occurs, but the
    registerParameter() method only returns false on failure. Sorry about that.

    For more registration examples, check out the documentation and source code for specific algorithms (eg.
    SideCar::Algorithms::NOOP).

    <h2>Status Reporting</h2>

    Algorithms can report status to external entities such as the Monitor program. Status information is
    contained in an XML-RPC container, which is filled in by the call Algorithm::getInfoData() method. The
    return value from this method should be a string value that identifies the source of the XML-RPC data.
    Usually, this should be the same name as the algorithm.
*/
class Algorithm {
public:
    /** Obtain a C-safe value for the given QString string. If string is empty, returns NULL. Otherwise,
        allocates a char buffer and fills it with UTF8-encoded contents of the string.

        \param value string to encode

        \return NULL or pointer to heap array holding UTF8-encoded string
    */
    static void* FormatInfoValue(const QString& value);

    /** Obtain a C-safe value for the given character string. If string is empty, returns NULL. Otherwise,
        allocates a char buffer and fills it with contents of the string.

        \param value string to encode

        \return NULL or pointer to heap array holding UTF8-encoded string
    */
    static void* FormatInfoValue(const char* value);

    /** Constructor.

        \param controller the processor controller in use

        \param log device to use for log messages for this processor
    */
    Algorithm(Controller& controller, Logger::Log& log);

    /** Destructor.
     */
    virtual ~Algorithm();

    /** Obtain the Controller object managing the algorithm.

        \return Controller reference
    */
    Controller& getController() const { return controller_; }

    /** Obtain the log device to use for by the algorithm for log messages.

        \return reference to Log device
    */
    Logger::Log& getLog() const { return log_; }

    /** Obtain the name of the algorithm. This is the name assigned in the XML configuration file, and may be
        different than the DLL name.

        \return
    */
    const std::string& getName() const { return controller_.getAlgorithmName(); }

    /** Add a configuration parameter to the runtime system. A parameter value may change during runtime via
        external messages to the processing module.

        \param parameter object to add

        \return true if successful
    */
    bool registerParameter(const Parameter::Ref& parameter) { return controller_.registerParameter(parameter); }

    /** Remove a configuration parameter from the runtime system.

        \param parameter object to remove

        \return true if successful
    */
    bool unregisterParameter(const Parameter::Ref& parameter) {
        return controller_.unregisterParameter(parameter);
    }

    /** Install a method to process a particular message type. Used by derived classes to install message
        processing methods. The two template parameters are

        - A -- name of the class derived from Algorithm that is registering
        - M -- name of the message class to register

        Accepts as its sole argument a pointer to a method which will be invoked by the registered processor.
        The method must take as its only parameter a boost::shared_ptr<T> object and return a boolean value to
        indicate success (true) or failure (false).

        All registrations shall appear in the startup() method. For example, to register a Video message
        processor in a class called Foo, the following would suffice:

        \code
        bool
        Foo::startup() {
            registerProcessor<Foo,Messages::Video>(&Foo::process);
            return true;
        }

        bool
        Foo::process(const Messages::Video::Ref& msg)
        {
        ...
        }
        \endcode

        \param proc pointer to a method to invoke to process a message
    */
    template <typename A, typename M>
    void registerProcessor(boost::function<bool(A*, typename M::Ref)> proc)
    {
        addProcessor(M::GetMetaTypeInfo(), new TProcessor<A, M>(static_cast<A*>(this), proc));
    }

    /** Same as above method, but allows user to pick the channel by name. Useful when there are multiple inputs
        with the same message type.

        \param channelName name of the input channel to use

        \param proc pointer to a method to invoke to process a message
    */
    template <typename A, typename M>
    void registerProcessor(const std::string& channelName, boost::function<bool(A*, typename M::Ref)> proc)
    {
        addProcessor(channelName, M::GetMetaTypeInfo(), new TProcessor<A, M>(static_cast<A*>(this), proc));
    }

    /** Same as above method, but allows user to pick the channel by index. Useful when there are multiple
        inputs with the same message type.

        \param channelIndex index of the input channel to use

        \param proc pointer to a method to invoke to process a message
    */
    template <typename A, typename M>
    void registerProcessor(size_t channelIndex, boost::function<bool(A*, typename M::Ref)> proc)
    {
        addProcessor(channelIndex, M::GetMetaTypeInfo(), new TProcessor<A, M>(static_cast<A*>(this), proc));
    }

    template <typename A>
    void registerProcessor(const std::string& channelName, const Messages::MetaTypeInfo& metaTypeInfo,
                           boost::function<bool(A*, typename Messages::Header::Ref)> proc)
    {
        addProcessor(channelName, metaTypeInfo, new TProcessor<A, Messages::Header>(static_cast<A*>(this), proc));
    }

    template <typename A>
    void registerProcessor(size_t channelIndex, const Messages::MetaTypeInfo& metaTypeInfo,
                           boost::function<bool(A*, typename Messages::Header::Ref)> proc)
    {
        addProcessor(channelIndex, metaTypeInfo,
                     new TProcessor<A, Messages::Header>(static_cast<A*>(this), proc));
    }

    /** Add a message processor for a channel. Locates the channel with a given name.

        \param channelName name of the channel to locate

        \param metaTypeInfo message type to process

        \param processor method to install
    */
    size_t addProcessor(const std::string& channelName, const Messages::MetaTypeInfo& metaTypeInfo,
                        Processor* processor);

    /** Add a message processor for a channel. Locates the first channel with a given message type.

        \param metaTypeInfo message type to process

        \param processor method to install
    */
    size_t addProcessor(const Messages::MetaTypeInfo& metaTypeInfo, Processor* processor);

    /** Add a message processor for a channel. Uses a specific channel obtained from Task::getInputChannel().

        \param index position of the channel to use

        \param metaTypeInfo message type to process

        \param processor method to install
    */
    size_t addProcessor(size_t index, const Messages::MetaTypeInfo& metaTypeInfo, Processor* processor);

    /** Remove a processor from a given channel. NOTE: any future messages received on a channel with no
        registered processor will cause the Controller hosting to algorithm to abort.

        \param index position of the channel to use (not verified)
    */
    void removeProcessor(size_t index);

    /** Obtain an output channel index with a given name. NOTE: throws an exception if the name is not found.

        \param name the name to look for

        \return the index found
    */
    size_t getOutputChannelIndex(const std::string& name) const {
        return controller_.getOutputChannelIndex(name);
    }

    /** Submit a message object to the controller for sending out on a given data channel. Assigns a unique
        sequence number to the message.

        NOTE: once passed to the send() method, the algorithm must not change the message since other threads
        will have access to it.

        WARNING: this is not thread-safe; a multi-threaded algorithm must not call send() simultaneously from
        multiple threads.

        \param msg object to send

        \return true if successful, false otherwise
    */
    bool send(const Messages::Header::Ref& msg, size_t channelIndex = 0);

    /** Hand a message to the controller for sending out on a given data channel. Unlike send() above, this does
        not update the message sequence counter.

        NOTE: once passed to the send() method, the algorithm must not change the message since other threads
        will have access to it.

        NOTE: unlike send() above, this method is thread-safe.

        \param msg object to send

        \return true if successful, false otherwise
    */
    bool sendAsIs(const Messages::Header::Ref& msg, size_t channelIndex = 0)
    {
        return controller_.send(msg, channelIndex);
    }

    /** Obtain the number of data slots found in the ControllerStatus XML array. Any custom Algorithm slots will
        appear after the last defined ControllerStatus slot. NOTE: implementations must be thread-safe, since
        this routine will execute in the alorithm's StatusEmitter thread.

        \return ControllerStats::kNumSlots
    */
    virtual size_t getNumInfoSlots() const { return ControllerStatus::kNumSlots; }

    /** Obtain custom status information from an algorithm. All algorithms report generic status information to
        external entities via the ControllerStatus object. However, an algorithm with special reporting needs
        can provide custom data by appending to array whatever values it wants. NOTE: implementations must be
        thread-safe, since this routine will execute in the algorithm's StatusEmitter thread.

        \param array container to hold the custom status information
    */
    virtual void setInfoSlots(IO::StatusBase& status) {}

    /** Obtain the index of the input channel that last received a message for the algorithm to process. This
        value is valid until the process() method returns.

        \return channel index
    */
    size_t getActiveChannelIndex() const { return activeChannelIndex_; }

protected:
    /** This method can be used by an Algorithm to schedule a periodic alarm to go off for itself.
     */
    void setAlarm(int secs);

    /** Hook called once after the DLL is loaded and before processing begins. If a service fails to start up
        for any reason, it should return false. Any configurable parameters must be registered here. The
        processor's config file will be loaded after this routine returns.

        \return true if successful.
    */
    virtual bool startup() { return true; }

    /** Hook called when the algorithm is transitioning from a stopped to a running state (one of
        Task::kAutoDiagnostic, Task::kCalibrate, or Task::kRun). Derived classes should override this method and
        perform whatever actions necessary to return the algorithm to a known state.

        \return true if successful
    */
    virtual bool reset() { return true; }

    /** Hook called when the algorithm is commaned to clear any statistics.

        \return true if successful
    */
    virtual bool clearStats() { return true; }

    /** Hook called when the algorithm is entering the Task::kAutoDiagnostic state.

        \return true if successful
    */
    virtual bool beginAutoDiag() { return true; }

    /** Hook called when the algorithm is entering the Task::kCalibrate state.

        \return true if successful
    */
    virtual bool beginCalibration() { return true; }

    /** Hook called when the algorithm is entering the Task::kRun state.

        \return true if successful
    */
    virtual bool beginRun() { return true; }

    /** Hook called when the algorithm is entering the Task::kStop state.

        \return true if successful
    */
    virtual bool stop() { return true; }

    /** Hook called when the processing system shuts down. Derived classes should perform any necessary cleanup
        not automatically handled by object destruction.
    */
    virtual bool shutdown() { return true; }

    /** Hook called when a recording has started. Default method does nothing.
     */
    virtual void recordingStarted() {}

    /** Hook called when a recording has stopped. Default method does nothing.
     */
    virtual void recordingStopped() {}

    /** Hook called just before algorithm runtime parameters are changed.
     */
    virtual void beginParameterChanges() {}

    /** Hook called just after algorithm runtime parameters have been changed.
     */
    virtual void endParameterChanges() {}

    /** This method will get called whenever an alarm expires for this Algorithm. Derived classes can override
        this method. If not, it's a no-op.
    */
    virtual void processAlarm() {}

private:
    /** Process a data message. Dispatches to the registered procedure for the given channel index.

        \param msg the message to process

        \param channelIndex channel associated with the message

        \return true if successful
    */
    bool process(const Messages::Header::Ref& msg, size_t channelIndex);

    Controller& controller_; ///< Controller for the processor
    Logger::Log& log_;       ///< Log device for log messages
    ProcessorVector processors_;
    std::vector<Messages::MetaTypeInfo::SequenceType> sequenceNumbers_;
    size_t activeChannelIndex_;
    vsip::vsipl vpp_;

    friend class Controller;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
