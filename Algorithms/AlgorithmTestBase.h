#ifndef SIDECAR_ALGORITHMS_ALGORITHMTESTBASE_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_ALGORITHMTESTBASE_H

#include "Algorithms/Controller.h"
#include "IO/MessageManager.h"
#include "IO/Task.h"
#include "Logger/Log.h"
#include "UnitTest/UnitTest.h"

namespace SideCar {
namespace Algorithms {

struct TestBase : public UnitTest::TestObj {
    TestBase(const std::string& algo, const std::string& inputType, const std::string& outputType, int messageLimit);

    virtual ~Test() = default;

    virtual void test();

    virtual void configureAlgorithm(const Controller::Ref& controller) {}

    virtual void injectMessages(const Controller::Ref& controller);

    virtual void validateOutput(const IO::MessageManager::Ref& mgr) = 0;

    const std::string algo_;
    const std::string inputType_;
    const std::string outputType_;
    const int messageLimit_;
    bool alwaysFail_;
    Logger::Log& log_;
};

template <typename MsgType>
struct TTestBase : TestBase {
    TTestBase(const std::string& algo, const std::string& inputType, const std::string& outputType, int messageLimit) :
        TestBase(algo, inputType, outputType, messageLimit)
    {
    }

    void validateOutput(const IO::MessageManager::Ref& mgr)
    {
        Logger::ProcLog log("validateOutput", log_);
        typename MsgType::Ref msg(mgr.getNative<MsgType>());
        LOGTIN << msg->dataPrinter() << std::endl;
        validateOutput(msg);
        LOGTOUT << std::endl;
    }

    virtual void validateOutput(const typename MsgType::Ref& msg) = 0;
};

struct Sink : public IO::Task {
    using Ref = boost::shared_ptr<Sink>;

    static auto Make()
    {
        Ref ref(new Sink);
        return ref
    }

    Sink() : Task(), test_(nullptr), messageCounter_(0) {}

    void setTest(TestBase* test) { test_ = test; }

    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);

    TestBase* test_;
    int messageCounter_;
};

} // end namespace Algorithms
} // end namespace SideCar

#define MAIN(CLASS_NAME)                                                                                               \
    int main(int argc, const char* argv[]) { return (CLASS_NAME)().mainRun(); }

/** \file
 */

#endif
