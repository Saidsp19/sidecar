#include "QtXml/QDomDocument"
#include "QtXml/QDomElement"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

#include "Logger/Log.h"
#include "UnitTest/UnitTest.h"

#include "RadarConfig.h"

using namespace SideCar;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj
{
    Test() : TestObj("RadarConfig") {}

    void test();
};

void
Test::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    assertFalse(RadarConfig::SetConfigurationFilePath("/a/b/c.xml"));

    RadarConfig::Load("blah", 1, 2, 3, 4, 5, 6);
    assertEqual("blah", RadarConfig::GetName());
    assertEqual(1U, RadarConfig::GetGateCountMax());
    assertEqual(2U, RadarConfig::GetShaftEncodingMax());
    assertEqual(3, RadarConfig::GetRotationRate());
    assertEqual(4.0, RadarConfig::GetRangeMin_deprecated());
    assertEqual(5.0, RadarConfig::GetRangeMax());
    assertEqual(6.0, RadarConfig::GetBeamWidth());
    assertEqual(20.0, RadarConfig::GetRotationDuration());

    {
	QString xml("<?xml version=\"1.0\" encoding=\"UTF=8\"?>\
<!DOCTYPE sidecar>\
<sidecar version=\"1.0\">\
  <info>\
    <name>Sample Configuration</name>\
    <comments>\
      \"Blah, blah, blah, got a pillow stuck in my head\" -- Tom Verlaine\
    </comments>\
  </info>\
  <radar>\
    <name>Big John</name>\
    <gateCountMax type=\"int\">4000</gateCountMax>\
    <shaftEncodingMax type=\"int\">4095</shaftEncodingMax>\
    <rotationRate units=\"rpm\" type=\"double\">6</rotationRate>\
    <rangeMin units=\"meters\" type=\"double\">5.0</rangeMin>\
    <rangeMax units=\"meters\" type=\"double\">300.0</rangeMax>\
    <beamWidth units=\"radians\" type=\"double\">0.001544</beamWidth>\
  </radar>\
  <riu>\
  </riu>\
  <rib>\
  </rib>\
  <dp config-path=\"/opt/sidecar/data\" recording-path=\"/space/recordings\">\
  </dp>\
</sidecar>");

	QDomDocument doc;
	assertTrue(doc.setContent(xml));
	QDomNode radar(doc.elementsByTagName("radar").at(0));

	assertTrue(RadarConfig::Load(radar.toElement()));
	assertEqual("Big John", RadarConfig::GetName());
	assertEqual(4000U, RadarConfig::GetGateCountMax());
	assertEqual(4095U, RadarConfig::GetShaftEncodingMax());
	assertEqual(6, RadarConfig::GetRotationRate());
	assertEqual(5.0, RadarConfig::GetRangeMin_deprecated());
	assertEqual(300, RadarConfig::GetRangeMax());
	assertEqual(0.001544, RadarConfig::GetBeamWidth());
	assertEqual(10.0, RadarConfig::GetRotationDuration());
    }

    {
	QString xml("<?xml version=\"1.0\" encoding=\"UTF=8\"?>\
<!DOCTYPE sidecar>\
<sidecar version=\"1.0\">\
  <info>\
    <name>Sample Configuration</name>\
    <comments>\
      \"Blah, blah, blah, got a pillow stuck in my head\" -- Tom Verlaine\
    </comments>\
  </info>\
  <radar>\
    <name>Yahoo</name>\
    <gateCountMax type=\"int\">4123</gateCountMax>\
    <shaftEncodingMax type=\"int\">1234</shaftEncodingMax>\
    <rotationRate units=\"rpm\" type=\"double\">5</rotationRate>\
    <rangeMin units=\"meters\" type=\"double\">8.0</rangeMin>\
    <rangeMax units=\"meters\" type=\"double\">400.0</rangeMax>\
    <beamWidth units=\"radians\" type=\"double\">0.003088</beamWidth>\
  </radar>\
  <riu>\
  </riu>\
  <rib>\
  </rib>\
  <dp config-path=\"/opt/sidecar/data\" recording-path=\"/space/recordings\">\
  </dp>\
</sidecar>");

	QDomDocument doc;
	assertTrue(doc.setContent(xml));
	QDomNode radar(doc.elementsByTagName("radar").at(0));

	assertTrue(RadarConfig::Load(radar.toElement()));
	assertEqual("Yahoo", RadarConfig::GetName());
	assertEqual(4123U, RadarConfig::GetGateCountMax());
	assertEqual(1234U, RadarConfig::GetShaftEncodingMax());
	assertEqual(5.0, RadarConfig::GetRotationRate());
	assertEqual(8.0, RadarConfig::GetRangeMin_deprecated());
	assertEqual(400, RadarConfig::GetRangeMax());
	assertEqual(0.003088, RadarConfig::GetBeamWidth());
	assertEqual(12.0, RadarConfig::GetRotationDuration());
    }
}

int
main(int, const char**)
{
    return Test().mainRun();
}
