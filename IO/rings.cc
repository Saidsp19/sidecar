#include <cmath>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>

#include "ace/FILE_Connector.h"

#include "IO/MessageManager.h"
#include "IO/Writers.h"
#include "Messages/RadarConfig.h"
#include "Messages/Video.h"
#include "Utils/CmdLineArgs.h"
#include "Utils/Utils.h"

using namespace SideCar;

const std::string about = "Simple PRI pattern generator.";

const Utils::CmdLineArgs::OptionDef options[] = {{'a', "arc", "number of radials on/off in ring", "ARC"},
                                                 {'c', "count", "number of rings to generate", "RINGS"},
                                                 {'f', "frequency", "rate at which the timmestamps change ", "FREQ"},
                                                 {'g', "gates", "number of samples per radial", "GATES"},
                                                 {'m', "minrange", "minimum range", "MIN"},
                                                 {'M', "maxrange", "maximum range", "MAX"},
                                                 {'n', "noise", "noise amplitude (0-32767)", "NOISE"},
                                                 {'p', "power", "value to use for ON", "POWER"},
                                                 {'r', "radials", "number of radials in a scan", "RADS"},
                                                 {'s', "scans", "number of scans to generate", "SCANS"},
                                                 {'P', "spacing", "spacing between rings", "WIDTH"},
                                                 {'w', "width", "width of ring in samples", "WIDTH"}};

const Utils::CmdLineArgs::ArgumentDef args[] = {{"FILE", "path to output file"}};

int
makeNoise(int level)
{
    return int(::drand48() * level - level / 2);
}

int
main(int argc, char** argv)
{
    size_t numScans = 1;
    size_t numGates = Messages::RadarConfig::GetGateCountMax();
    size_t numRadials = Messages::RadarConfig::GetRadialCount();
    double minRange = Messages::RadarConfig::GetRangeMin_deprecated();
    double maxRange = Messages::RadarConfig::GetRangeMax();
    size_t numRings = 10;
    size_t ringWidth = 5;
    size_t arcWidth = numRadials;
    double frequency = 360.0;

    int noise = 1024;
    int power = 8192;

    Utils::CmdLineArgs cla(argc, argv, about, options, sizeof(options), args, sizeof(args));
    std::string value;

    if (cla.hasOpt("frequency", value))
        if (!(value >> frequency) || frequency <= 0.0) cla.usage("invalid 'frequency' value");

    if (cla.hasOpt("minrange", value))
        if (!(value >> minRange) || minRange < 0.0) cla.usage("invalid 'minrange' value");

    if (cla.hasOpt("maxrange", value))
        if (!(value >> maxRange) || maxRange < 1.0) cla.usage("invalid 'maxrange' value");

    if (minRange >= maxRange) cla.usage("min range must be less than max range");

    if (cla.hasOpt("scans", value))
        if (!(value >> numScans) || numScans < 1) cla.usage("invalid 'scans' value");

    if (cla.hasOpt("gates", value))
        if (!(value >> numGates) || numGates < 1) cla.usage("invalid 'samples' value");

    if (cla.hasOpt("radials", value)) {
        if (!(value >> numRadials) || numRadials < 360) cla.usage("invalid 'radials' value");
        arcWidth = numRadials;
    }

    if (cla.hasOpt("count", value))
        if (!(value >> numRings) || numRings < 1 || numRings > numGates) cla.usage("invalid 'count' value");

    if (cla.hasOpt("width", value))
        if (!(value >> ringWidth) || ringWidth < 1 || ringWidth > 100) cla.usage("invalid 'width' value");

    if (cla.hasOpt("arc", value))
        if (!(value >> arcWidth) || arcWidth < 1 || arcWidth > numRadials) cla.usage("invalid 'arc' value");

    if (cla.hasOpt("power", value)) {
        if (!(value >> power) || power < 1 || power > 32767) {
            std::ostringstream os;
            os << "invalid 'power' value - " << value;
            cla.usage(os.str());
        }
    }

    if (cla.hasOpt("noise", value)) {
        if (!(value >> noise)) cla.usage("invalid 'noise' value");
        power -= noise;
    }

    double timeRate = 1.0 / frequency;
    double beamWidth = (M_PI * 2.0) / numRadials;
    double shaftChange = double(Messages::RadarConfig::GetShaftEncodingMax()) / numRadials;
    double ringSpacing = double(numGates) / double(numRings);

    if (cla.hasOpt("spacing", value))
        if (!(value >> ringSpacing)) cla.usage("invalid 'spacing' value");

    std::clog << "numRadials: " << numRadials << " beamWidth: " << Utils::radiansToDegrees(beamWidth) << " degrees "
              << beamWidth << " radians\n";
    std::clog << "shaft change: " << shaftChange << '\n';
    std::clog << "numGates: " << numGates << " minRange: " << minRange << " maxRange: " << maxRange << '\n';
    std::clog << "numRings: " << numRings << " width: " << ringWidth << " arc: " << arcWidth
              << " spacing: " << ringSpacing << '\n';
    std::clog << "time change: " << timeRate << " frequency: " << frequency << std::endl;

    // Create a new FileWriter object, and then 'connect' it to the file path given on the command line -- this
    // opens the file.
    //
    ::unlink(cla.arg(0).c_str());
    IO::FileWriter::Ref writer(IO::FileWriter::Make());
    ACE_FILE_Addr address(cla.arg(0).c_str());
    ACE_FILE_Connector fd(writer->getDevice(), address);

    double clock = 0.0;
    bool arcVisible = true;
    size_t arcCounter = arcWidth;

    Messages::VMEDataMessage vme;
    vme.header.msgDesc = ((Messages::VMEHeader::kPackedReal << 16) | Messages::VMEHeader::kAzimuthValidMask |
                          Messages::VMEHeader::kIRIGValidMask | Messages::VMEHeader::kPRIValidMask);
    vme.header.pri = 0;
    vme.header.timeStamp = 0;
    vme.rangeMin = minRange;
    vme.rangeFactor = (maxRange - minRange) / numGates;

    // Generate the requested number of scans
    //
    double shaftSim = 0.0;
    for (size_t scan = 0; scan < numScans; ++scan) {
        // Generate the requested number of radials
        //
        for (size_t radial = 0; radial < numRadials; ++radial) {
            ++vme.header.pri;
            vme.header.azimuth = uint32_t(::rint(shaftSim));
            shaftSim += shaftChange;
            if (shaftSim > Messages::RadarConfig::GetShaftEncodingMax())
                shaftSim -= Messages::RadarConfig::GetShaftEncodingMax();

            Messages::Video::Ref msg(Messages::Video::Make("rings", vme, numGates));

            msg->setCreatedTimeStamp(clock);
            clock += timeRate;
            vme.header.irigTime = clock;

            if (!arcVisible) {
                // This gate is hidden, so just write noise
                //
                for (size_t gate = 0; gate < numGates; ++gate) msg->push_back(makeNoise(noise));
            } else {
                // The gate is potentially visible. See if it should be for this radial.
                //
                size_t onCounter = 0;
                double nextRing = ringSpacing - ringWidth;
                if (nextRing < 0.0) nextRing = 0.0;
                for (size_t gate = 0; gate < numGates; ++gate) {
                    if (gate >= nextRing) {
                        nextRing += ringSpacing;
                        onCounter = ringWidth;
                    }

                    msg->push_back(makeNoise(noise) + (onCounter ? power : 0));
                    if (onCounter) --onCounter;
                }
            }

            if (--arcCounter == 0) {
                arcVisible = !arcVisible;
                arcCounter = arcWidth;
            }

            // Done creating the PRI mesage. Write its CDR representation out to disk.
            //
            IO::MessageManager mgr(msg);
            writer->writeEncoded(1, mgr.getEncoded());
        }
    }

    return 0;
}
