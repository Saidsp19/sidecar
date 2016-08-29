#include "UnitTest/UnitTest.h"

#include "MessageManager.h"
#include "ParametersChangeRequest.h"
#include "ProcessingStateChangeRequest.h"
#include "RecordingStateChangeRequest.h"

using namespace SideCar;
using namespace SideCar::IO;

struct ParametersChangeRequestTest : public UnitTest::TestObj
{
    ParametersChangeRequestTest() : TestObj("ParametersChangeRequest") {}
    void test();
};

void
ParametersChangeRequestTest::test()
{
    XmlRpc::XmlRpcValue input;
    input["one"] = 1;
    input["two"] = 2;
    input["three"] = 3;
    input["four"] = new XmlRpc::XmlRpcValue::ValueArray;
    input["four"].push_back("a");
    input["four"].push_back("b");
    input["four"].push_back("c");
    input["four"].push_back("d");

    ACE_Message_Block* wrapped;
    {
	ParametersChangeRequest request(input, false);
	wrapped = request.getWrapped();
	assertEqual(2, wrapped->reference_count());
	wrapped->release();
	wrapped = request.getWrapped();
	assertEqual(2, wrapped->reference_count());
	assertEqual(false, request.hasOriginalValues());
    }

    assertEqual(1, wrapped->reference_count());
    assertEqual(ControlMessage::kParametersChange + MessageManager::kControl,
                wrapped->msg_type());

    XmlRpc::XmlRpcValue output;
    {
	ParametersChangeRequest request(wrapped);
	output = request.getValue();
	assertEqual(false, request.hasOriginalValues());
    }

    assertEqual(XmlRpc::XmlRpcValue::TypeStruct, output.getType());
    assertEqual(1, int(output["one"]));
    assertEqual(2, int(output["two"]));
    assertEqual(3, int(output["three"]));
    assertEqual(XmlRpc::XmlRpcValue::TypeArray, output["four"].getType());
}

struct ProcessingStateChangeRequestTest : public UnitTest::TestObj
{
    ProcessingStateChangeRequestTest()
	: TestObj("ProcessingStateChangeRequest") {}
    void test();
};

void
ProcessingStateChangeRequestTest::test()
{
    ProcessingState::Value input = ProcessingState::kRun;
    ACE_Message_Block* wrapped;
    {
	ProcessingStateChangeRequest request(input);
	wrapped = request.getWrapped();
	assertEqual(2, wrapped->reference_count());
	wrapped->release();
	wrapped = request.getWrapped();
	assertEqual(2, wrapped->reference_count());
    }

    assertEqual(1, wrapped->reference_count());
    assertEqual(ControlMessage::kProcessingStateChange +
                MessageManager::kControl, wrapped->msg_type());

    IO::ProcessingState::Value output;
    {
	ProcessingStateChangeRequest request(wrapped);
	output = request.getValue();
    }

    assertEqual(input, output);
}

struct RecordingStateChangeRequestTest : public UnitTest::TestObj
{
    RecordingStateChangeRequestTest()
	: TestObj("RecordingStateChangeRequest") {}
    void test();
};

void
RecordingStateChangeRequestTest::test()
{
    std::string input("/a/b/c/d/e/f/g");
    ACE_Message_Block* wrapped;
    {
	RecordingStateChangeRequest request(input);
	wrapped = request.getWrapped();
	assertEqual(2, wrapped->reference_count());
	wrapped->release();
	wrapped = request.getWrapped();
	assertEqual(2, wrapped->reference_count());
    }

    assertEqual(1, wrapped->reference_count());
    assertEqual(ControlMessage::kRecordingStateChange +
                MessageManager::kControl, wrapped->msg_type());

    std::string output("");
    {
	RecordingStateChangeRequest request(wrapped);
	assertTrue(request.isOn());
	output = request.getValue();
    }

    assertEqual(input, output);

    {
	RecordingStateChangeRequest request("");
	wrapped = request.getWrapped();
    }
    {
	RecordingStateChangeRequest request(wrapped);
	assertFalse(request.isOn());
	output = request.getValue();
    }

    assertEqual(std::string(""), output);
}

int
main(int, const char**)
{
    UnitTest::Suite suite("ControlMessage");
    suite.add(new ParametersChangeRequestTest);
    suite.add(new ProcessingStateChangeRequestTest);
    suite.add(new RecordingStateChangeRequestTest);
    return suite.mainRun();
}
