#include <cmath>

#include "ace/OS_Memory.h"

#include "Logger/Log.h"
#include "UnitTest/UnitTest.h"

#include "Extraction.h"

using namespace SideCar;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj
{
    Test() : TestObj("Extractions") {}

    void test();
};

struct ExtractionLayout
{
    /** Align on 4-byte boundary
     */
    int32_t seconds;
    int32_t microSeconds;

    /** Align on 8-byte boundary (already is)
     */
    double range;
    double azimuth;
    double elevation;
    double x;
    double y;

    /** Attributes string - length byte followed by that number of characters.
     */
    uint32_t count;

    // char buffer[count];
};

struct ExtractionsLayout
{
    // uint16_t magic;		// 0xAAAA
    // uint16_t alignment;	// Zero is little-endian
    // uint32_t payload;	// Number of bytes in message

    uint16_t headerVersion;
    uint16_t guidVersion;
    uint32_t producerLength;
    char     producer[4];	// !!!
    uint16_t messageTypeKey;
    uint32_t sequenceNumber;
    int32_t createdTimeStampSeconds;
    int32_t createdTimeStampMicroSeconds;
    int32_t emittedTimeStampSeconds;
    int32_t emittedTimeStampMicroSeconds;

    // Align on 4-byte boundary
    //
    uint32_t count;
    ExtractionLayout first;
};

void
Test::test()
{
    // Since our message layout is used by Ed Bowker's software, implement a test case that will fail when the
    // layout changes.
    //
    Extractions::Ref extractions(Extractions::Make("test", Header::Ref()));

    {
	Extraction extraction(Time::TimeStamp(1, 2),
                              3.45678, // range
                              4.56789, // azimuth
                              5.67890 // elevation
            );
	extraction.addAttribute("foo", "one");
	extractions->push_back(extraction);
    }

    {
	Extraction extraction(Time::TimeStamp(3, 4),
                              5.67890, // range
                              4.56789, // azimuth
                              3.45678 // elevation
            );

	extraction.addAttribute("foo", "two");
	extractions->push_back(extraction);
    }

    ACE_OutputCDR output;
    extractions->write(output);

    ExtractionsLayout* ptr = reinterpret_cast<ExtractionsLayout*>(
	const_cast<char*>(output.buffer()));

    assertEqual(2, ptr->headerVersion);
    assertEqual(3, ptr->guidVersion);
    assertEqual(4, ptr->producerLength);

    assertEqual(2, ptr->count);
    assertEqual(std::string("test"),
                std::string(ptr->producer, ptr->producerLength));

    // Validate values in the first Extraction record.
    //
    ExtractionLayout* rec = &ptr->first;

    assertEqual(1, rec->seconds);
    assertEqual(2, rec->microSeconds);
    assertEqual(3.45678, rec->range);
    assertEqual(4.56789, rec->azimuth);
    assertEqual(5.67890, rec->elevation);
    assertEqual(rec->range * ::sin(rec->azimuth), rec->x);
    assertEqual(rec->range * ::cos(rec->azimuth), rec->y);
    assertEqual(83, rec->count);

    // Obtain a pointer to the first character of the attribute string.
    //
    char* cptr = reinterpret_cast<char*>(&rec->count + 1);
    cptr[rec->count] = 0;
    assertEqual("<value><struct><member><name>foo</name>"
                "<value>one</value></member></struct></value>",
                cptr);

    // Now move to the next record. This is tricky since we have to align to a 4-byte boundary after skipping
    // the attribute string data.
    //
    cptr += rec->count + 1;	// account for a null byte
    cptr = ACE_ptr_align_binary(cptr, 4);

    // Validate values in the second Extraction record
    //
    rec = reinterpret_cast<ExtractionLayout*>(cptr);

    assertEqual(3, rec->seconds);
    assertEqual(4, rec->microSeconds);
    assertEqual(5.67890, rec->range);
    assertEqual(4.56789, rec->azimuth);
    assertEqual(3.45678, rec->elevation);
    assertEqual(rec->range * ::sin(rec->azimuth), rec->x);
    assertEqual(rec->range * ::cos(rec->azimuth), rec->y);
    assertEqual(83, rec->count);

    // Obtain a pointer to the first character of the attribute string.
    //
    cptr = reinterpret_cast<char*>(&rec->count + 1);
    cptr[rec->count] = 0;
    assertEqual("<value><struct><member><name>foo</name>"
                "<value>two</value></member></struct></value>",
                cptr);
}

int
main(int, const char**)
{
    return Test().mainRun();
}
