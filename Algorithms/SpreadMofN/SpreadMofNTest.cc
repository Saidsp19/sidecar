#include "ace/FILE_Connector.h"
#include "ace/Reactor.h"

#include "Algorithms/Controller.h"
#include "Algorithms/ShutdownMonitor.h"
#include "IO/Readers.h"
#include "IO/FileWriterTask.h"
#include "IO/MessageManager.h"
#include "IO/Module.h"
#include "IO/ParametersChangeRequest.h"
#include "IO/ProcessingStateChangeRequest.h"
#include "IO/ShutdownRequest.h"
#include "IO/Stream.h"
#include "IO/Task.h"

#include "Logger/Log.h"
#include "Messages/BinaryVideo.h"
#include "Parameter/Parameter.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"
#include "XMLRPC/XmlRpcValue.h"

#include "SpreadMofN.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

#include <iostream>
using std::cout;
using std::endl;

struct Test : public UnitTest::TestObj
{
    Test() : UnitTest::TestObj("SpreadMofN") {}
    void test();
};

void
Test::test()
{	
    // Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);
    Utils::TemporaryFilePath testOutputPath("SpreadMofNTest");
    {
	Stream::Ref stream(Stream::Make("test"));
	
	assertEqual(0, stream->push(new ShutdownMonitorModule(stream)));
	FileWriterTaskModule* writer = new FileWriterTaskModule(stream);

	assertEqual(0, stream->push(writer));
	assertTrue(writer->getTask()->openAndInit("BinaryVideo", testOutputPath));

	ControllerModule* controllerMod = new ControllerModule(stream);
        assertEqual(0, stream->push(controllerMod));
        Controller::Ref controller = controllerMod->getTask();
        controller->setTaskIndex(0);

        controller->addInputChannel(Channel(Task::Ref(), "one", "BinaryVideo"));

        assertTrue(controller->openAndInit("SpreadMofN"));

        stream->put(ProcessingStateChangeRequest(
                       ProcessingState::kRun).getWrapped());

        SpreadMofN* spreadMofN = dynamic_cast<SpreadMofN*>(
            controller->getAlgorithm());


	assertTrue(spreadMofN);
	
	BinaryVideo::DatumType data[41][5] =
            {
		// CPI # 1
		{ false, false, false, false, false },
		{ false, false, false, false, false },
		{ true,  false, false, false, false }, // begin interesting portion of CPI
		{ true,  true,  false, false, false },
		{ true,  true,  true,  false, false },
		{ true,  true,  true,  true,  false },
		{ true,  true,  true,  true,  true  }, // end interesting portion of CPI
		{ false, false, false, false, false },
		// CPI # 2
		{ false, false, false, false, false },
		{ false, false, false, false, false },
		{ true,  false, false, false, false }, // begin interesting portion of CPI
		{ true,  true,  false, false, false },
		{ true,  true,  true,  false, false },
		{ true,  true,  true,  true,  false },
		{ true,  true,  true,  true,  true  }, // end interesting portion of CPI
		{ false, false, false, false, false },
		// CPI # 3
		{ false, false, false, false, false },
		{ false, false, false, false, false },
		{ true,  false, false, false, false }, // begin interesting portion of CPI
		{ true,  true,  false, false, false },
		{ true,  true,  true,  false, false },
		{ true,  true,  true,  true,  false },
		{ true,  true,  true,  true,  true  }, // end interesting portion of CPI
		{ false, false, false, false, false },
		// CPI # 4
		{ false, false, false, false, false },
		{ false, false, false, false, false },
		{ true,  false, false, false, false }, // begin interesting portion of CPI
		{ true,  true,  false, false, false },
		{ true,  true,  true,  false, false },
		{ true,  true,  true,  true,  false },
		{ true,  true,  true,  true,  true  }, // end interesting portion of CPI
		{ false, false, false, false, false },
		// CPI # 5
		{ false, false, false, false, false }, 
		{ false, false, false, false, false }, 
		{ true,  false, true,  false, false }, // begin interesting portion of CPI
		{ false, true,  true,  true,  true  },
		{ false, false, false, true,  true  },
		{ true,  true,  false, true,  false },
		{ true,  false, true,  true,  false }, // end interesting portion of CPI
		{ false, false, false, false, false },

		{ false, false, false, false, false }
            };

	int M[5] = { 5, 4, 3, 2, 3 };
		
	VMEDataMessage vme;

	XmlRpc::XmlRpcValue parameterChange;
	parameterChange.setSize(4);
	parameterChange[0] = "cpiSpan";
	parameterChange[1] = 8;
	parameterChange[2] = "M";
	parameterChange[3] = 0;

	// Because CPIAlgorithm needs the first message of the next CPI before passing buffer to processCPI(),
	// we have to add the next message of the next CPI before changing to the next parameter.
	//
	BinaryVideo::Ref msg(BinaryVideo::Make("test", vme, (BinaryVideo::DatumType *) data, 
                                               (BinaryVideo::DatumType*) data + 5));
	msg->getRIUInfo().prfEncoding = 1;
	MessageManager init_mgr(msg);
	stream->put(init_mgr.getMessage(), 0);
	assertFalse(init_mgr.hasEncoded());

	BinaryVideo::DatumType * ptr = (BinaryVideo::DatumType*) data + 5;

	for(int i = 0; i < 5; i++) {
	  
            // configure the algorithm's parameters
            parameterChange[3] = M[i];
            controller->injectControlMessage(
                ParametersChangeRequest(parameterChange, false));
            // pass in the input data
            //
            for(int j = 1; j < 8; j++) {
		msg = BinaryVideo::Make("test", vme, ptr, ptr + 5);
		ptr += 5;
		msg->getRIUInfo().prfEncoding = i + 1;
		MessageManager mgr(msg);
		stream->put(mgr.getMessage(), 0);
		assertFalse(mgr.hasEncoded());
            }

            // trigger processCPI() call
            //
            msg = BinaryVideo::Make("test", vme, ptr, ptr + 5);
            ptr += 5;
            msg->getRIUInfo().prfEncoding = i + 2;
            MessageManager trigger_mgr(msg);
            stream->put(trigger_mgr.getMessage(), 0);
            assertFalse(trigger_mgr.hasEncoded());

	}

	stream->put(ShutdownRequest().getWrapped());
	ACE_Reactor::instance()->run_reactor_event_loop();

	writer->getTask()->close(1);
    }

    BinaryVideo::DatumType results[5][5] = 
	{
            { true, false, false, false, false }, // M = 5
            { true, true,  false, false, false }, // M = 4
            { true, true,  true,  false, false }, // M = 3
            { true, true,  true,  true,  false }, // M = 2
            { true, false, true,  true,  false }  // M = 3
	};
  
    FileReader::Ref reader(new FileReader);
    ACE_FILE_Addr inputAddr(testOutputPath);
    ACE_FILE_Connector inputConnector(reader->getDevice(), inputAddr);

    // Iterate through the resulting test results and validate them.
    //
    for(int i = 0; i < 5; i++) {
	assertTrue(reader->fetchInput());
	assertTrue(reader->isMessageAvailable());
	{
            Decoder decoder(reader->getMessage());
            BinaryVideo::Ref msg(decoder.decode<BinaryVideo>());
            assertEqual(size_t(5), msg->size());
            BinaryVideo::const_iterator pos = msg->begin();
	  
            for(size_t j = 0; j < 5; j++) {
		assertEqual(results[i][j], *pos++);
            }
	  
            assertTrue(pos == msg->end());
	}
    }

    assertFalse(reader->fetchInput());

    // Uncomment the following to fail the test and see the log results.
    //
    //assertTrue(false);
  
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}

