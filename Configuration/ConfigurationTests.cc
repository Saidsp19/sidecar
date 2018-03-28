#include <fstream>

#include "Logger/Log.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "Loader.h"
#include "RunnerConfig.h"

using namespace SideCar::Configuration;

struct ConfigurationTests : public UnitTest::ProcSuite<ConfigurationTests> {
    ConfigurationTests() : UnitTest::ProcSuite<ConfigurationTests>(this, "Configuration")
    {
        add("Normal", &ConfigurationTests::testNormal);
        add("FailedFileOpen", &ConfigurationTests::testFailedFileOpen);
        add("FailedXMLParse", &ConfigurationTests::testFailedXMLParse);
        add("MissingSidecar", &ConfigurationTests::testMissingSidecar);
        add("MissingRadar", &ConfigurationTests::testMissingRadar);
        add("InvalidRadar", &ConfigurationTests::testInvalidRadar);
        add("MissingDP", &ConfigurationTests::testMissingDP);
        add("InvalidIncludes", &ConfigurationTests::testIncludes);
    }

    void testNormal();
    void testFailedFileOpen();
    void testFailedXMLParse();
    void testMissingSidecar();
    void testMissingRadar();
    void testInvalidRadar();
    void testMissingDP();
    void testIncludes();
};

void
ConfigurationTests::testNormal()
{
    Utils::TemporaryFilePath tempFilePath;
    {
        const char* text = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<sidecar>\
 <radar>\
  <name>Radar</name>\
  <gateCountMax type=\"int\">4684</gateCountMax>\
  <shaftEncodingMax type=\"int\">65535</shaftEncodingMax>\
  <rotationRate units=\"rpm\" type=\"double\">6</rotationRate>\
  <rangeMin units=\"km\" type=\"double\">0.0</rangeMin>\
  <rangeMax units=\"km\" type=\"double\">300.0</rangeMax>\
  <beamWidth units=\"radians\" type=\"double\">0.00125644</beamWidth>\
 </radar>\
 <dp recordingsDirectory=\"/a/b\" logsDirectory=\"/c\">\
  <runner name=\"First\" host=\"alpha\" multicast=\"237.1.2.101\">\
   <stream name=\"One\">\
    <subscriber name=\"Hello\"/>\
   </stream>\
  </runner>\
 </dp>\
</sidecar>\
";
        std::ofstream os(tempFilePath.filePath().c_str());
        os << text;
    }

    Loader loader;
    assertEqual(Loader::kNotLoaded, loader.getLastLoadResult());
    assertFalse(loader.isLoaded());

    assertTrue(loader.load(QString::fromStdString(tempFilePath)));
    assertEqual(Loader::kOK, loader.getLastLoadResult());
    assertEqual(1, loader.getRunnerConfigs().size());
    assertEqual("/a/b", loader.getRecordingsDirectory().toStdString());
    assertEqual("/c", loader.getLogsDirectory().toStdString());
}

void
ConfigurationTests::testFailedFileOpen()
{
    Loader loader;
    assertFalse(loader.load("/this should/ not match/ a file path"));
    assertEqual(Loader::kFailedFileOpen, loader.getLastLoadResult());
}

void
ConfigurationTests::testFailedXMLParse()
{
    Utils::TemporaryFilePath tempFilePath;
    {
        const char* text = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<sidecar>\n\
 <dp recordingsDirectory=\"/a/b\" logsDirectory=\"/c\">\n\
  <runner name=\"First\" host=\"alpha\" multicast=\"237.1.2.101\">\n\
   <stream name=\"One\">\n\
    <subscriber name=\"Hello\">\n\
   </stream>\n\
  </runner>\n\
 </dp>\n\
</sidecar>\n\
";
        std::ofstream os(tempFilePath.filePath().c_str());
        os << text;
    }

    Loader loader;
    assertFalse(loader.load(QString::fromStdString(tempFilePath)));
    assertEqual(Loader::kFailedXMLParse, loader.getLastLoadResult());
    assertEqual(tempFilePath.getFilePath().filePath(), loader.getParseFilePath().toStdString());
    int line;
    int column;
    QString error = loader.getParseErrorInfo(line, column);
    assertEqual(7, line);
    assertEqual(12, column);
}

void
ConfigurationTests::testMissingSidecar()
{
    Utils::TemporaryFilePath tempFilePath;
    {
        const char* text = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<foobar>\
</foobar>\
";
        std::ofstream os(tempFilePath.filePath().c_str());
        os << text;
    }

    Loader loader;
    assertFalse(loader.load(QString::fromStdString(tempFilePath)));
    assertEqual(Loader::kMissingSidecarNode, loader.getLastLoadResult());
}

void
ConfigurationTests::testMissingRadar()
{
    Utils::TemporaryFilePath tempFilePath;
    {
        const char* text = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<sidecar>\
 <dp recordingsDirectory=\"/a/b\" logsDirectory=\"/c\">\
  <runner name=\"First\" host=\"alpha\" multicast=\"237.1.2.101\">\
   <stream name=\"One\">\
    <subscriber name=\"Hello\"/>\
   </stream>\
  </runner>\
 </dp>\
</sidecar>\
";
        std::ofstream os(tempFilePath.filePath().c_str());
        os << text;
    }

    Loader loader;
    assertFalse(loader.load(QString::fromStdString(tempFilePath)));
    assertEqual(Loader::kMissingRadarNode, loader.getLastLoadResult());
}

void
ConfigurationTests::testInvalidRadar()
{
    Utils::TemporaryFilePath tempFilePath;
    {
        const char* text = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<sidecar>\
 <radar>\
  <name>Radar</name>\
  <gateCountMax type=\"int\">4684</gateCountMax>\
  <shaftEncodingMaxXXX type=\"int\">65535</shaftEncodingMaxXXX>\
  <rotationRate units=\"rpm\" type=\"double\">6</rotationRate>\
  <rangeMin units=\"km\" type=\"double\">0.0</rangeMin>\
  <rangeMax units=\"km\" type=\"double\">300.0</rangeMax>\
  <beamWidth units=\"radians\" type=\"double\">0.00125644</beamWidth>\
 </radar>\
 <dp recordingsDirectory=\"/a/b\" logsDirectory=\"/c\">\
  <runner name=\"First\" host=\"alpha\" multicast=\"237.1.2.101\">\
   <stream name=\"One\">\
    <subscriber name=\"Hello\"/>\
   </stream>\
  </runner>\
 </dp>\
</sidecar>\
";
        std::ofstream os(tempFilePath.filePath().c_str());
        os << text;
    }

    Loader loader;
    assertEqual(Loader::kNotLoaded, loader.getLastLoadResult());
    assertFalse(loader.isLoaded());
    assertFalse(loader.load(QString::fromStdString(tempFilePath)));
    assertEqual(Loader::kInvalidRadarNode, loader.getLastLoadResult());
}

void
ConfigurationTests::testMissingDP()
{
    Utils::TemporaryFilePath tempFilePath;
    {
        const char* text = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<sidecar>\
 <radar>\
  <name>Radar</name>\
  <gateCountMax type=\"int\">4684</gateCountMax>\
  <shaftEncodingMax type=\"int\">65535</shaftEncodingMax>\
  <rotationRate units=\"rpm\" type=\"double\">6</rotationRate>\
  <rangeMin units=\"km\" type=\"double\">0.0</rangeMin>\
  <rangeMax units=\"km\" type=\"double\">300.0</rangeMax>\
  <beamWidth units=\"radians\" type=\"double\">0.00125644</beamWidth>\
 </radar>\
 <dppp recordingsDirectory=\"/a/b\" logsDirectory=\"/c\">\
  <runner name=\"First\" host=\"alpha\" multicast=\"237.1.2.101\">\
   <stream name=\"One\">\
    <subscriber name=\"Hello\"/>\
   </stream>\
  </runner>\
 </dppp>\
</sidecar>\
";
        std::ofstream os(tempFilePath.filePath().c_str());
        os << text;
    }

    Loader loader;
    assertFalse(loader.load(QString::fromStdString(tempFilePath)));
    assertEqual(Loader::kMissingDPNode, loader.getLastLoadResult());
}

void
ConfigurationTests::testIncludes()
{
    Utils::TemporaryFilePath mainFilePath;
    Utils::TemporaryFilePath radarFilePath;
    Utils::TemporaryFilePath runner1FilePath;
    Utils::TemporaryFilePath runner2FilePath;
    {
        std::ofstream os(mainFilePath.filePath().c_str());
        os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           << "<sidecar>\n"
           << " <radar file=\"" << radarFilePath << "\"/>\n"
           << " <dp recordingsDirectory=\"/a/b\" logsDirectory=\"/c\">\n"
           << "  <runner name=\"A\" file=\"" << runner1FilePath << "\"\n"
           << "    host=\"alpha\" multicast=\"237.1.2.101\"/>\n"
           << "  <runner file=\"" << runner2FilePath << "\"/>\n"
           << " </dp>\n</sidecar>\n";
    }

    {
        std::ofstream os(radarFilePath.filePath().c_str());
        os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           << "<radar>\n"
           << " <name>Radar</name>\n"
           << " <gateCountMax type=\"int\">4684</gateCountMax>\n"
           << " <shaftEncodingMax type=\"int\">65535</shaftEncodingMax>\n"
           << " <rotationRate units=\"rpm\" type=\"double\">6</rotationRate>\n"
           << " <rangeMin units=\"km\" type=\"double\">0.0</rangeMin>\n"
           << " <rangeMax units=\"km\" type=\"double\">300.0</rangeMax>\n"
           << " <beamWidth units=\"radians\" type=\"double\">0.01</beamWidth>\n"
           << "</radar>\n";
    }

    {
        std::ofstream os(runner1FilePath.filePath().c_str());
        os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           << "<runner name=\"A2\" host=\"alpha2\" multicast=\"999.9.9.999\">\n"
           << " <stream name=\"One\">\n"
           << "  <subscriber name=\"Hello\"/>\n"
           << " </stream>\n"
           << "</runner>\n";
    }

    {
        std::ofstream os(runner2FilePath.filePath().c_str());
        os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           << "<runner name=\"B\" host=\"beta\" multicast=\"999.9.9.999\">\n"
           << " <stream name=\"Two\">\n"
           << "  <subscriber name=\"Bye\"/>\n"
           << " </stream>\n"
           << "</runner>\n";
    }

    ::setenv("SIDECAR", "/opt/sidecar", 1);

    Loader loader;
    assertEqual(Loader::kNotLoaded, loader.getLastLoadResult());
    assertFalse(loader.isLoaded());

    assertTrue(loader.load(QString::fromStdString(mainFilePath)));
    assertEqual(Loader::kOK, loader.getLastLoadResult());
    assertEqual(2, loader.getRunnerConfigs().size());
    assertEqual("/a/b", loader.getRecordingsDirectory().toStdString());
    assertEqual("/c", loader.getLogsDirectory().toStdString());
    const RunnerConfig* runnerConfig = loader.getRunnerConfig("A2");
    assertTrue(runnerConfig == 0);
    runnerConfig = loader.getRunnerConfig("A");
    assertTrue(runnerConfig != 0);
    assertEqual(std::string("alpha"), runnerConfig->getHostName().toStdString());
    assertEqual(std::string("237.1.2.101"), runnerConfig->getMulticastAddress().toStdString());
    assertEqual(1, runnerConfig->getStreamNodes().size());

    // Since the Loader used the basename component of the configuration file for its configuration name, we get
    // "TemporaryFilePath" in the results because we use a TemporaryFilePath to hold our config file.
    //
    assertEqual(std::string("TemporaryFilePath:alpha:A"), runnerConfig->getServiceName().toStdString());
    assertEqual(std::string("/c/TemporaryFilePath_A.log"), runnerConfig->getLogPath().toStdString());
    std::string cmd("ssh -Tfnx alpha '/usr/bin/nohup "
                    "\"/opt/sidecar/bin/startup\" -L "
                    "\"/c/TemporaryFilePath_A.log\" "
                    "runner  \"A\" \"");
    cmd += mainFilePath.getFilePath().filePath();
    cmd += "\"'";

    assertEqual(cmd, runnerConfig->getRemoteCommand().toStdString());

    runnerConfig = loader.getRunnerConfig("B");
    assertTrue(runnerConfig != 0);
    assertEqual(std::string("beta"), runnerConfig->getHostName().toStdString());
    assertEqual(std::string("999.9.9.999"), runnerConfig->getMulticastAddress().toStdString());
    assertEqual(1, runnerConfig->getStreamNodes().size());
}

int
main(int argc, const char* argv[])
{
    return ConfigurationTests().mainRun();
}
