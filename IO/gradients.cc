#include <cmath>
#include <iostream>
#include <string>

#include "ace/FILE_Connector.h"

#include "IO/MessageManager.h"
#include "IO/Writers.h"
#include "Messages/RadarConfig.h"
#include "Messages/Video.h"
#include "Utils/CmdLineArgs.h"
#include "Utils/Utils.h"

using namespace SideCar;

const std::string about = "Simple PRI gradient generator.";

const Utils::CmdLineArgs::OptionDef options[] = {
    {'g', "gates", "number of samples per radial", "GATES"},
    {'f', "frequency", "rate at which the timmestamps change ", "FREQ"},
    {'r', "radials", "number of radials in a scan", "RADS"},
    {'m', "minrange", "minimum range", "RANGE"},
    {'M', "maxrange", "maximum range", "RANGE"},
    {'p', "minpower", "minimum power", "POWER"},
    {'P', "maxpower", "maximum power", "POWER"},
    {'S', "powerstep", "change in power levels", "DELTA"},
    {'s', "scans", "number of scans to generate", "SCANS"},
};

const Utils::CmdLineArgs::ArgumentDef args[] = {{"FILE", "path to output file"}};

int
main(int argc, char** argv)
{
    size_t numScans = 1;
    size_t numGates = Messages::RadarConfig::GetGateCountMax();
    size_t numRadials = Messages::RadarConfig::GetRadialCount();
    double minRange = Messages::RadarConfig::GetRangeMin_deprecated();
    double maxRange = Messages::RadarConfig::GetRangeMax();
    double minPower = 0.0;
    double maxPower = 16000.0;
    double powerStep = 1000.0;
    double frequency = 360.0;

    Utils::CmdLineArgs cla(argc, argv, about, options, sizeof(options), args, sizeof(args));
    std::string value;

    if (cla.hasOpt("frequency", value))
        if (!(value >> frequency) || frequency <= 0.0) cla.usage("invalid 'frequency' value");

    if (cla.hasOpt("minrange", value))
        if (!(value >> minRange) || minRange < 0.0) cla.usage("invalid 'minrange' value");

    if (cla.hasOpt("maxrange", value))
        if (!(value >> maxRange) || maxRange < 1.0) cla.usage("invalid 'maxrange' value");

    if (minRange >= maxRange) cla.usage("min range must be less than max range");

    if (cla.hasOpt("minpower", value))
        if (!(value >> minPower) || minPower < -32768) cla.usage("invalid 'minpower' value");

    if (cla.hasOpt("maxpower", value))
        if (!(value >> maxPower) || maxPower > 32767) cla.usage("invalid 'maxpower' value");

    if (cla.hasOpt("powerstep", value))
        if (!(value >> powerStep) || powerStep < 1.0) cla.usage("invalid 'powerstep' value");

    if (minPower >= maxPower) cla.usage("min power must be less than max power");

    if (cla.hasOpt("scans", value))
        if (!(value >> numScans) || numScans < 1) cla.usage("invalid 'scans' value");

    if (cla.hasOpt("gates", value))
        if (!(value >> numGates) || numGates < 1) cla.usage("invalid 'samples' value");

    if (cla.hasOpt("radials", value))
        if (!(value >> numRadials) || numRadials < 360) cla.usage("invalid 'radials' value");

    double timeRate = 1.0 / frequency;
    double beamWidth = M_PI * 2.0 / numRadials;

    std::clog << "numRadials: " << numRadials << " beamWidth: " << Utils::radiansToDegrees(beamWidth) << '\n';
    std::clog << "numGates: " << numGates << " minRange: " << minRange << " maxRange: " << maxRange << '\n';
    std::clog << "minPower: " << minPower << " maxPower: " << maxPower << " powerStep: " << powerStep << '\n';
    std::clog << "timeRate: " << timeRate << " frequency: " << frequency << std::endl;

    // Create a new FileWriter object, and then 'connect' it to the file path given on the command line -- this
    // opens the file.
    //
    IO::FileWriter::Ref writer(IO::FileWriter::Make());
    ACE_FILE_Addr address(cla.arg(0).c_str());
    ACE_FILE_Connector fd(writer->getDevice(), address);

    Time::TimeStamp clock(0.0);
    double powerLevel = minPower;

    Messages::VMEDataMessage vme;
    vme.header.msgDesc = ((Messages::VMEHeader::kPackedReal << 16) | Messages::VMEHeader::kIRIGValidMask |
                          Messages::VMEHeader::kAzimuthValidMask | Messages::VMEHeader::kPRIValidMask);
    vme.header.pri = 0;
    vme.header.timeStamp = 0;
    vme.rangeMin = minRange;
    vme.rangeFactor = (maxRange - minRange) / numGates;

    // Generate the requested number of scans
    //
    for (size_t scan = 0; scan < numScans; ++scan) {
        std::clog << "...scan: " << scan << std::endl;

        // Generate the requested number of radials
        //
        for (size_t radial = 0; radial < numRadials; ++radial) {
            ++vme.header.pri;
            vme.header.azimuth =
                uint32_t(::rint(radial * beamWidth / (M_PI * 2.0) * Messages::RadarConfig::GetShaftEncodingMax()));

            Messages::Video::Ref msg(Messages::Video::Make("gradients", vme, numGates));
            msg->setCreatedTimeStamp(clock);
            clock += timeRate;
            msg->resize(numGates, int(::rint(powerLevel)));
            if (powerLevel + powerStep > maxPower || powerLevel + powerStep < minPower) powerStep *= -1;
            powerLevel += powerStep;
            IO::MessageManager mgr(msg);
            writer->write(mgr);
        }
    }

    return 0;
}
