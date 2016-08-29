#include "ace/FILE_Connector.h"
#include "ace/Reactor.h"
#include "ace/Stream.h"

#include "IO/Channel.h"
#include "IO/Readers.h"
#include "IO/FileWriterTask.h"
#include "IO/MessageManager.h"

#include "Logger/Log.h"
#include "Messages/PRIMessage.h"
#include "Parameter/Parameter.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "Inverter.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj
{
    Test() : UnitTest::TestObj("Inverter") {}
    void test();
};

void
Test::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);
    Utils::TemporaryFilePath testOutputPath("inverterTestOutput");
    {
	ACE_Stream<ACE_MT_SYNCH> stream;
	
	Module<FileWriterTask>* writer =
	    new Module<FileWriterTask>("Writer");
	assertEqual(0, stream.push(writer));
	assertTrue(writer->getTask()->open(0, testOutputPath));

	Module<Controller>* controller =
	    new Module<Controller>("Inverter");
	assertEqual(0, stream.push(controller));
	assertTrue(controller->getTask()->open("Inverter"));

	Inverter* inverter = dynamic_cast<Inverter*>(
	    controller->getTask()->getAlgorithm());
	assertTrue(inverter);
	inverter->setMin(1);
	inverter->setMax(10);

	int16_t init[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	PRIMessage::Ref msg(PRIMessage::Make(0.0, 1.0, 0.0, 300.0, 3,
                                             init, init + 10));
	MessageManager mgr(msg, 0);
	stream.put(mgr.getMessage(), 0);
	assertFalse(mgr.hasEncoded());
    }

    FileReader::Ref reader(new FileReader);
    ACE_FILE_Addr inputAddr(testOutputPath.c_str());
    ACE_FILE_Connector inputConnector(reader->getDevice(), inputAddr);
    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    Decoder decoder(reader->getMessage());
    {
	PRIMessage::Ref msg(decoder.decode<PRIMessage>());
	assertEqual(size_t(10), msg->size());
	PRIMessage::const_iterator pos = msg->begin();
	assertEqual(10, *pos++);
	assertEqual(9, *pos++);
	assertEqual(8, *pos++);
	assertEqual(7, *pos++);
	assertEqual(6, *pos++);
	assertEqual(5, *pos++);
	assertEqual(4, *pos++);
	assertEqual(3, *pos++);
	assertEqual(2, *pos++);
	assertEqual(1, *pos++);
	assertTrue(pos == msg->end());
    }
    
    // Uncomment the following to fail the test and see the log results. assertTrue(false);
}

int
main(int argc, const char* argv[])
{
    printf("hello world\n");
    return Test().mainRun();
}
