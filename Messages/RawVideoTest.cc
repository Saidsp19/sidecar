#include <cmath>
#include <netinet/in.h>

#include "ace/FILE_Connector.h"

#include "IO/Decoder.h"
#include "IO/MessageManager.h"
#include "IO/Readers.h"
#include "IO/Writers.h"
#include "Logger/Log.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "RawVideo.h"
#include "VMEHeader.h"
#include "Video.h"

using namespace SideCar;
using namespace SideCar::Messages;
using namespace SideCar::Time;

struct Test : public UnitTest::TestObj {
    Test() : TestObj("Video") {}

    void test();
};

void
Test::test()
{
    const int kNumSamples = 10;

    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    size_t size = sizeof(VMEDataMessage) + sizeof(int16_t) * (kNumSamples - 1);
    ACE_Message_Block* data = new ACE_Message_Block(size);
    VMEDataMessage* vme = reinterpret_cast<VMEDataMessage*>(data->wr_ptr());

    vme->header.msgSize = size;
    vme->header.msgDesc = (VMEHeader::kUnpackedReal << 16) + VMEHeader::kTimeStampValidMask +
                          VMEHeader::kAzimuthValidMask + VMEHeader::kPRIValidMask;
    if (ACE_CDR_BYTE_ORDER == 1) vme->header.msgDesc += VMEHeader::kEndianessMask;

    vme->header.msgDesc = htonl(vme->header.msgDesc);

    vme->header.timeStamp = 1;
    vme->header.azimuth = 2;
    vme->header.pri = 3;
    vme->header.irigTime = 4.567;
    vme->rangeMin = 8.9;
    vme->rangeFactor = (300.0 - 8.9) / 10;
    vme->numSamples = kNumSamples;
    for (int index = 0; index < kNumSamples; ++index) {
        int16_t value = index * index;
        vme->samples[index] = ~(value << 2);
    }

    data->wr_ptr(size);

    ACE_OutputCDR output(size_t(0), ACE_CDR_BYTE_ORDER);

    {
        RawVideo::Ref raw(RawVideo::Make("test", data));
        assertTrue(raw->write(output).good_bit());
        Video::Ref video(raw->convert("scooby"));
        assertEqual(size_t(kNumSamples), video->size());
        for (int index = 0; index < kNumSamples; ++index) assertEqual(index * index, video[index]);
    }
    {
        ACE_InputCDR input(output);
        RawVideo::Ref raw(RawVideo::Make(input));
        Video::Ref video(raw->convert("dooby"));
        assertEqual(size_t(kNumSamples), video->size());
        for (int index = 0; index < kNumSamples; ++index) assertEqual(index * index, video[index]);
    }
}

int
main(int, const char**)
{
    return Test().mainRun();
}
