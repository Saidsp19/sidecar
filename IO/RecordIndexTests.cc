#include "ace/FILE_Connector.h"

#include "Logger/Log.h"
#include "Messages/VMEHeader.h"
#include "Messages/Video.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "IndexMaker.h"
#include "MessageManager.h"
#include "Readers.h"
#include "RecordIndex.h"
#include "TimeIndex.h"
#include "Writers.h"

using namespace SideCar;
using namespace SideCar::IO;

struct Test : public UnitTest::TestObj
{
    Test() : TestObj("RecordIndex") {}
    void test();
};

void
Test::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    // Create a temporary file to use for testing.
    //
    Utils::TemporaryFilePath fp1;
    ACE_FILE_Addr addr(fp1);

    // Write out 100 Video messages
    //
    {
	Messages::VMEDataMessage vme;
	vme.header.msgDesc = ((Messages::VMEHeader::kPackedReal << 16) |
                              Messages::VMEHeader::kIRIGValidMask |
                              Messages::VMEHeader::kAzimuthValidMask |
                              Messages::VMEHeader::kPRIValidMask);
	vme.header.pri = 0;
	vme.header.timeStamp = 0;
	vme.rangeMin = 0;
	vme.rangeFactor = 1.0;
	FileWriter::Ref writer(FileWriter::Make());
	ACE_FILE_Connector(writer->getDevice(), addr);

	for (int count = 0; count < 100; ++count) {
	    ++vme.header.pri;
	    Messages::Video::Ref msg(Messages::Video::Make("Test", vme, 0));
	    MessageManager mgr(msg);
	    assertTrue(writer->write(mgr.getMessage()));
	}
    }

    // Create a new index files
    //
    IndexMaker::Make(fp1.filePath(), 1);

    // And remove them when we exit scope
    //
    Utils::FilePath timeIndexFilePath(fp1.filePath());
    timeIndexFilePath.setExtension(TimeIndex::GetIndexFileSuffix());
    Utils::TemporaryFilePath fp2(timeIndexFilePath, false);

    Utils::FilePath recordIndexFilePath(fp1.filePath());
    recordIndexFilePath.setExtension(RecordIndex::GetIndexFileSuffix());
    Utils::TemporaryFilePath fp3(recordIndexFilePath, false);
    
    RecordIndex recordIndex(recordIndexFilePath);

    // Simple tests for overall capacity.
    //
    assertEqual(100U, recordIndex.size());

    // Load in the record pointed to in the last entry. It should be the 100th record written out.
    //
    FileReader::Ref reader(FileReader::Make());
    ACE_FILE_Connector(reader->getDevice(), addr);
    {
	reader->getDevice().seek(recordIndex[0], SEEK_SET);
	assertTrue(reader->fetchInput());
	assertTrue(reader->isMessageAvailable());
	MessageManager mgr(reader->getMessage(), &Messages::Video::GetMetaTypeInfo());
	Messages::Header::Ref msg(mgr.getNative());
	assertEqual(1U, msg->getGloballyUniqueID().getMessageSequenceNumber());
    }
    {
	reader->getDevice().seek(recordIndex[1], SEEK_SET);
	assertTrue(reader->fetchInput());
	assertTrue(reader->isMessageAvailable());
	MessageManager mgr(reader->getMessage(), &Messages::Video::GetMetaTypeInfo());
	Messages::Header::Ref msg(mgr.getNative());
	assertEqual(2U, msg->getGloballyUniqueID().getMessageSequenceNumber());
    }
    {
	reader->getDevice().seek(recordIndex[recordIndex.size() - 1], SEEK_SET);
	assertTrue(reader->fetchInput());
	assertTrue(reader->isMessageAvailable());
	MessageManager mgr(reader->getMessage(), &Messages::Video::GetMetaTypeInfo());
	Messages::Header::Ref msg(mgr.getNative());
	assertEqual(100U, msg->getGloballyUniqueID().getMessageSequenceNumber());
    }
	
    // !!! Uncomment to see debug messages even if all tests above succeed. assertFalse(true);
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
