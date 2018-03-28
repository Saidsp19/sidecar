#include "ace/FILE_Connector.h"
#include <cmath>
#include <sstream>

#include "IO/Decoder.h"
#include "IO/MessageManager.h"
#include "IO/Readers.h"
#include "IO/Writers.h"
#include "Logger/Log.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"
#include "Utils/Utils.h"

#include "RadarConfig.h"
#include "TSPI.h"

using namespace SideCar;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj {
    Test() : TestObj("TSPI") {}

    void test();
};

void
Test::test()
{
    // Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    double siteLatitude = RadarConfig::GetSiteLatitude();
    double siteLongitude = RadarConfig::GetSiteLongitude();
    double siteHeight = RadarConfig::GetSiteHeight();

    double targetLatitude = siteLatitude;
    double targetLongitude = siteLongitude;
    double targetHeight = siteHeight;

    TSPI::Ref msg = TSPI::MakeLLH("Test", "site", 0.0, targetLatitude, targetLongitude, targetHeight);

    assertEqual(Utils::degreesToRadians(targetLatitude), msg->getLatitude());
    assertEqual(Utils::degreesToRadians(targetLongitude), msg->getLongitude());
    assertEqual(targetHeight, msg->getHeight());

    // Test CDR streaming. First write TSPI objects to a file.
    //
    Utils::TemporaryFilePath fp("tspiTestOutput");
    ACE_FILE_Addr addr(fp);
    {
        IO::FileWriter::Ref writer(IO::FileWriter::Make());
        ACE_FILE_Connector fd(writer->getDevice(), addr);
        {
            IO::MessageManager mgr(msg);
            assertTrue(writer->write(mgr.getMessage()));
        }
        {
            msg = TSPI::MakeLLH("Test", "north", 0.0, targetLatitude + 10.0, targetLongitude, targetHeight);
            IO::MessageManager mgr(msg);
            assertTrue(writer->write(mgr.getMessage()));
        }
        {
            msg = TSPI::MakeLLH("Test", "south", 0.0, targetLatitude - 10.0, targetLongitude, targetHeight);
            IO::MessageManager mgr(msg);
            assertTrue(writer->write(mgr.getMessage()));
        }
        {
            msg = TSPI::MakeLLH("Test", "east", 0.0, targetLatitude, targetLongitude + 10.0, targetHeight);
            IO::MessageManager mgr(msg);
            assertTrue(writer->write(mgr.getMessage()));
        }
        {
            msg = TSPI::MakeLLH("Test", "west", 0.0, targetLatitude, targetLongitude - 10.0, targetHeight);
            IO::MessageManager mgr(msg);
            assertTrue(writer->write(mgr.getMessage()));
        }
        {
            msg = TSPI::MakeXYZ("Test", "xyz", 0.0, 0.0, 0.0, 0.0);
            IO::MessageManager mgr(msg);
            assertTrue(writer->write(mgr.getMessage()));
        }
    }

    // Now test reading TSPI objects from a file.
    //
    {
        IO::FileReader::Ref reader(IO::FileReader::Make());
        ACE_FILE_Connector fd(reader->getDevice(), addr);
        assertFalse(reader->isMessageAvailable());
        assertTrue(reader->fetchInput());
        assertTrue(reader->isMessageAvailable());
        double epsilon = 1.0E-8;
        {
            IO::Decoder decoder(reader->getMessage());
            msg = decoder.decode<TSPI>();
            assertEqual(std::string("site"), msg->getTag());
            assertEqualEpsilon(targetLatitude, Utils::radiansToDegrees(msg->getLatitude()), epsilon);
            assertEqualEpsilon(targetLongitude, Utils::radiansToDegrees(msg->getLongitude()), epsilon);
            assertEqualEpsilon(targetHeight, msg->getHeight(), epsilon);

            assertEqualEpsilon(0.0, msg->getRange(), epsilon);

            assertEqualEpsilon(0.0, Utils::radiansToDegrees(msg->getAzimuth()), epsilon);
            assertEqualEpsilon(0.0, Utils::radiansToDegrees(msg->getElevation()), epsilon);
        }

        assertFalse(reader->isMessageAvailable());
        assertTrue(reader->fetchInput());
        assertTrue(reader->isMessageAvailable());

        {
            IO::Decoder decoder(reader->getMessage());
            msg = decoder.decode<TSPI>();
            assertEqual(std::string("north"), msg->getTag());
            assertEqualEpsilon(targetLatitude + 10.0, Utils::radiansToDegrees(msg->getLatitude()), epsilon);
            assertEqualEpsilon(targetLongitude, Utils::radiansToDegrees(msg->getLongitude()), epsilon);
            assertEqualEpsilon(targetHeight, msg->getHeight(), epsilon);

            assertEqualEpsilon(1109485.013, msg->getRange(), 0.001);
            assertEqualEpsilon(0.0, Utils::radiansToDegrees(msg->getAzimuth()), 0.001);
            assertEqualEpsilon(354.998, // below the horizon
                               Utils::radiansToDegrees(msg->getElevation()), 0.001);
        }

        assertFalse(reader->isMessageAvailable());
        assertTrue(reader->fetchInput());
        assertTrue(reader->isMessageAvailable());

        {
            IO::Decoder decoder(reader->getMessage());
            msg = decoder.decode<TSPI>();
            assertEqual(std::string("south"), msg->getTag());
            assertEqualEpsilon(targetLatitude - 10.0, Utils::radiansToDegrees(msg->getLatitude()), epsilon);
            assertEqualEpsilon(targetLongitude, Utils::radiansToDegrees(msg->getLongitude()), epsilon);
            assertEqualEpsilon(targetHeight, msg->getHeight(), epsilon);

            assertEqualEpsilon(1107617.155, msg->getRange(), 0.001);
            assertEqualEpsilon(180.0, Utils::radiansToDegrees(msg->getAzimuth()), 0.001);
            assertEqualEpsilon(355.001, Utils::radiansToDegrees(msg->getElevation()), 0.001);
        }

        assertFalse(reader->isMessageAvailable());
        assertTrue(reader->fetchInput());
        assertTrue(reader->isMessageAvailable());

        {
            IO::Decoder decoder(reader->getMessage());
            msg = decoder.decode<TSPI>();
            assertEqual(std::string("east"), msg->getTag());
            assertEqualEpsilon(targetLatitude, Utils::radiansToDegrees(msg->getLatitude()), epsilon);
            assertEqualEpsilon(targetLongitude + 10.0, Utils::radiansToDegrees(msg->getLongitude()), epsilon);
            assertEqualEpsilon(targetHeight, msg->getHeight(), epsilon);

            assertEqualEpsilon(879363.732, msg->getRange(), 0.001);
            assertEqualEpsilon(86.929, Utils::radiansToDegrees(msg->getAzimuth()), 0.001);
            assertEqualEpsilon(356.0521, Utils::radiansToDegrees(msg->getElevation()), 0.001);
        }

        assertFalse(reader->isMessageAvailable());
        assertTrue(reader->fetchInput());
        assertTrue(reader->isMessageAvailable());

        {
            IO::Decoder decoder(reader->getMessage());
            msg = decoder.decode<TSPI>();
            assertEqual(std::string("west"), msg->getTag());
            assertEqualEpsilon(targetLatitude, Utils::radiansToDegrees(msg->getLatitude()), epsilon);
            assertEqualEpsilon(targetLongitude - 10.0, Utils::radiansToDegrees(msg->getLongitude()), epsilon);
            assertEqualEpsilon(targetHeight, msg->getHeight(), epsilon);

            assertEqualEpsilon(879363.7324, msg->getRange(), 0.001);
            assertEqualEpsilon(273.071, Utils::radiansToDegrees(msg->getAzimuth()), 0.001);
            assertEqualEpsilon(356.052, Utils::radiansToDegrees(msg->getElevation()), 0.001);
        }

        assertFalse(reader->isMessageAvailable());
        assertTrue(reader->fetchInput());
        assertTrue(reader->isMessageAvailable());

        {
            IO::Decoder decoder(reader->getMessage());
            msg = decoder.decode<TSPI>();
            assertEqual(std::string("xyz"), msg->getTag());
            assertEqualEpsilon(targetLatitude, Utils::radiansToDegrees(msg->getLatitude()), epsilon);
            assertEqualEpsilon(targetLongitude, Utils::radiansToDegrees(msg->getLongitude()), epsilon);
            assertEqualEpsilon(targetHeight, msg->getHeight(), epsilon);

            assertEqualEpsilon(0.0, msg->getRange(), epsilon);
            assertEqualEpsilon(0.0, msg->getAzimuth(), epsilon);
        }

        assertFalse(reader->isMessageAvailable());
        assertFalse(reader->fetchInput());
    }
}

int
main(int, const char**)
{
    return Test().mainRun();
}
