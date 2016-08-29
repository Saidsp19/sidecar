#ifndef SIDECAR_ALGORITHMS_DOXYGEN_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_DOXYGEN_H

/** \page algorithms Algorithms Overview

    \section new Creating a New Algorithm

    The simplest way to create a new algorithm is to first become familiar with existing ones, and then
    duplicate one that is similar to the task you wish to accomplish with the new algorithm.

    You may also use the newalg utility to create a set of C++ files that will get you started. To use, simply
    run the script with the name of the algorithm you would like to create:

    \code
    % newalg Foobar
    \endcode

    The newalg script creates a new directory for the algorithm in the Algorithms top-level directory. In the
    new directory, it places four files there:

    - \c Foobar.cc the C++ source file for the algorithm
    - \c Foobar.h the C++ include file for the algorithm
    - \c FoobarTest.cc a basic unit test for the algorithm
    - \c Jamfile Boost Build script for the algorithm

    The C++ source files implement a basic algorithm that simply adds 100 to each incoming video message sample.
    However, it is well-documented, and it provides much of what most algorithms will need to do their job.

    \section channels Data Channels

    Information flows into and out of an algorithm on a data channel, a named conduit that directs data flow
    among IO::Task objects in the same IO::Stream object. Data channels are an enhancement to the normal data
    flow mechanism found in ACE Stream and Task objects, which simply connects the output of one Task object to
    the input of its downstream neighbor. The SideCar enhancement allows for faster message delivery, since only
    recipients of a message will receive it, not every Task object downstream from the message emitter.

    Creation of data channels occurs at IO::Stream creation by the StreamBuilder class used by the Runner
    application. Channel names and types are assigned per the XML configuration file used by the Runner
    application to create and instantiate IO::Stream objects (the SideCar API allows for dynamic data channel
    creation, but doing so is not supported at this time)

    For input data channels, use the two-argument registerProcessor template method to assign a message
    processor to a named data channel. If there is no named channel at the time of the registration, an
    exception is thrown, indicating a disconnect between what the algorithm expects for inputs and what an XML
    input file has configured. For example, the side-lobe cancelling algorithm (see Algorithms::SLC) accepts
    data from a main video channel, and up to 8 auxillary channels. In its startup() method, it has the
    following processor registrations:

    \code
    registerProcessor<SLC,Messages::Video>("main", &SLC::main);
    registerProcessor<SLC,Messages::Video>("aux1", &SLC::aux1);
    registerProcessor<SLC,Messages::Video>("aux2", &SLC::aux2);
    registerProcessor<SLC,Messages::Video>("aux3", &SLC::aux3);
    registerProcessor<SLC,Messages::Video>("aux4", &SLC::aux4);
    registerProcessor<SLC,Messages::Video>("aux5", &SLC::aux5);
    registerProcessor<SLC,Messages::Video>("aux6", &SLC::aux6);
    registerProcessor<SLC,Messages::Video>("aux7", &SLC::aux7);
    registerProcessor<SLC,Messages::Video>("aux8", &SLC::aux8);
    \endcode

    An alternative way to do the above would be to use the channel introspection routines found in IO::Task to
    determine the name and number of the input channels defined, and to only register those channels, like so:

    \code
    for (size_t index = 0; index < getController().getNumInputChannels();
    ++index) {
    const Channel& channel(getController().getInputChannel(index));
    if (channel.getName() == "main")
    registerProcessor<SLC,Messages::Video>(index, &SLC::main);
    else if (channel.getName() == "aux1")
    registerProcessor<SLC,Messages::Video>(index, &SLC::aux1);
    else if (channel.getName() == "aux2")
    registerProcessor<SLC,Messages::Video>(index, &SLC::aux2);
    else if (channel.getName() == "aux3")
    registerProcessor<SLC,Messages::Video>(index, &SLC::aux3);
    else if (channel.getName() == "aux4")
    registerProcessor<SLC,Messages::Video>(index, &SLC::aux4);
    else if (channel.getName() == "aux5")
    registerProcessor<SLC,Messages::Video>(index, &SLC::aux5);
    else if (channel.getName() == "aux6")
    registerProcessor<SLC,Messages::Video>(index, &SLC::aux6);
    else if (channel.getName() == "aux7")
    registerProcessor<SLC,Messages::Video>(index, &SLC::aux7);
    else if (channel.getName() == "aux8")
    registerProcessor<SLC,Messages::Video>(index, &SLC::aux8);
    else
    return false;
    }
    \endcode

    Although the name checking above is tedious and slow, it is only performed at startup, where speed is not
    really an issue. The big benefit to the above is that the XML configuration does not need to define all
    eight auxillary data channels for the SLC algorithm to run; it only needs to define channels that will have
    data on them.

    \section parameters Runtime Parameters

    The SideCar framework contains a suite of templates and classes (see SideCar::Parameter) that allow for
    runtime configurable parameters. These Parameter objects have XML-RPC support so that their value may be
    changed by an external process, such as the SideCar::GUI::Master application. These objects also offer a
    signalling mechanism to notify algorithms when the Parameter's value changes.

    Like any other object attribute, a runtime parameter is declared in the algorithm's C++ header file,
    preferably at the bottom after the \c private: access declaration:

    \code
    private:
    Parameters::IntValue::Ref param1_;
    Parameters::DoubleValue::Ref param2_;
    \endcode

    The declaration must have a matching initializer in the algorithm's constructor:

    \code
    Foobar::Foobar(Controller& controller, Logger::Log& log)
    : Algorithm(controller, log),
    param1_(Parameter::IntValue::Make("param1", 123)),
    param2_(Parameter::IntValue::Make("param2", 3.1415926))
    {}
    \endcode
    
    The first argument to a parameter's Make factory method is the name the parameter will have when viewed by
    an external editor. It does not have to match the name of the parameter declared in the C++ header file;
    instead, it should be somewhat descriptive of what the value does in the algorithm. The second argument to
    the Make factor method is the initial value to give to the parameter at initialization time. Note that the
    XML configuration file used by the SideCar::GUI::Master application to start up algorithms may contain
    initialization values as well, and these values will override those given to the Make factory method.

    If an algorithm needs to know when a parameter value is changed, it must register a notification method to
    the parameter's change signal, preferably in the algorithm's constructor body:

    \code
    Foobar::Foobar(Controller& controller, Logger::Log& log)
    : Algorithm(controller, log),
    param1_(Parameter::IntValue::Make("param1", 123)),
    param2_(Parameter::IntValue::Make("param2", 3.1415926))
    {
    param1_->connectChangedSignalTo(boost::bind(&Foobar::param1Changed,
    this, _1));
    }
    \endcode

    The notification method takes as its sole argument a reference to the parameter whose value has changed.
    Here's the declaration for the param1Changed method refered to above:

    \code
    void param1Changed(const Parameter::IntValue& param);
    \endcode

    The SideCar API guarantees that parameter notification methods like param1Changed above will not run while
    an algorithm is processing a message. Thus, an algorithm is safe to manipulate its own state due to a
    parameter change without concern for protecting or locking state data from access by a processor method.

    \section status Runtime Status

    All Algorithm objects, through their Controller manager, report status information to external entities via
    the ControllerStatus object. The SideCar::GUI::Master application obtains periodic status updates from
    active Runner processes, and displays the status information to the user. The standard status display for an
    algorithm shows pending queue and recording queue sizes, current processing state, recording status (active,
    standby, or inactive), and any error text posted by the algorithm by way of Task::setError().
    
    Algorithms desiring customized reporting information, should override the getInfoData() method, which is
    called by the algorithm's Controller when it wants updated status information. The sole parameter is an
    XmlRpc::XmlRpcValue reference that may be used to store any valid XML-RPC value. The return value of the
    getInfoData() is a std::string that must be a unique value specific to the algorithm. The
    SideCar::GUI::Master application expects to have a GUI::Master::InfoFormatter object defined under the same
    name as that returned by the getInfoData() method. The formattter object will receive the data stored by the
    algorithm, and reformatted it into a text string for presentation to the user.

    A simplistic example of an specialized getInfoData() method appears below, where the Foobar algorithm echos
    the current value of its param1_ and param2_ runtime parameters in its status. Since there are two items,
    the routine starts by setting the given reference to size 2, which causes the reference to become an XML-RPC
    array:

    \code
    std::string
    Foobar::getInfoData(XmlRpc::XmlRpcValue& infoData)
    {
    infoData.setSize(2);
    infoData[0] = XmlRpc::XmlRpcValue(param1_->getValue());
    infoData[1] = XmlRpc::XmlRpcValue(param2_->getValue());
    return "Foobar";
    }
    \endcode

    Again, the SideCar::GUI::Master application needs an InfoFormatter object installed which understands how to
    format status information with the name "Foobar". See SideCar::GUI::Master documetation for instructions on
    how to do this.

    \section logging Logging

    The SideCar system contains a powerful logging facility. Each algorithm has its own log device, which it is
    given in the algorithm's constructor. The log device controls which log messages make it to a logging
    mechanism such as a file or a syslog daemon (see Logger::Log for the log device API).

    The canonical way to use the algorithm's log device is to pass it to a procedural logger, defined by
    Logger::ProcLog. A prodecural logger takes two arguments in its constructor:

    - \c name the routine name containing the procedural log device
    - \c logger the algorithm's log device (obtained from the getLog() method).

    If the procedural log device is named \c log then the following macros may be used in a routine to emit log
    messages at a particular log level:

    - \c LOGFATAL send a message with kFatal priority
    - \c LOGERROR send a message with kError priority
    - \c LOGWARNING send a message with kWarning priority
    - \c LOGINFO send a message with kInfo priority
    - \c LOGDEBUG send a message with kDebug priority

    Here's an example of an instrumented param1Changed method:

    \code
    void
    Foobar::param1Changed(const Parameter::IntValue& param)
    {
    Logger::ProcLog log("param1Changed", getLog());
    LOGINFO << "new value: " << param.getValue() << std::endl;
    if (param.getValue() < 1) {
    LOGERROR << "too small" << std::endl;
    param1_->setValue(1);
    }
    else if (param.getValue() > 10) {
    LOGERROR << "too big" << std::endl;
    param1_->setValue(10);
    }

    updateState();
    }
    \endcode

    The macros actually expand to an \c if statement with a condition that checks if the log device accepts a
    message at the indicated priority, and if so, the \c if body gets executed which contains the stream
    insertion operations. As a result, log messages below the reporting level of the algorithm's log device
    incur very little overhead. Feel free to instrument your algorithm code with copious amounts of LOGDEBUG and
    LOGINFO statements without worry. Normal SideCar behavior is to only emit log messages at kWarning or above.
*/

namespace SideCar {

/** Namespace for entities related to SideCar algorithms.

    For a description of the algorithm API, see the documentation for SideCar::Algorithms::Algorithm.h. The
    currently-defined algorithms are listed below.
*/
namespace Algorithms {}

} // end namespace SideCar

#endif
