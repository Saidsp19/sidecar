#include <cmath>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "ace/FILE_Connector.h"

#include "IO/MessageManager.h"
#include "IO/Writers.h"
#include "Messages/RadarConfig.h"
#include "Messages/Video.h"
#include "Utils/CmdLineArgs.h"
#include "Utils/Utils.h"

using namespace SideCar;

const std::string about = "Simple PRI sine generator.";

const Utils::CmdLineArgs::OptionDef options[] = {
    { 'c', "complex", "write out complex data", 0 },
    { 'f', "frequency", "rate at which the timestamps change", "PRF" },
    { 'g', "gates", "number of samples per radial", "GATES" },
    { 'm', "minrange", "minimum range", "RANGE" },
    { 'M', "maxrange", "maximum range", "RANGE" },
    { 'p', "minpower", "minimum power", "POWER" },
    { 'P', "maxpower", "maximum power", "POWER" },
    { 'r', "radials", "number of radials in a scan", "RADS" },
    { 's', "scans", "number of scans to generate", "SCANS" },
};

const Utils::CmdLineArgs::ArgumentDef args[] = {
    { "FILE", "path to output file" },
    { 0, 0 },
    { "SPEC", "waveform specification: CYCLE/AMP/DC/OFFSET" }
};

class Generator
{
public:

    Generator() {}

    Generator(int gatesPerCycle, double amplitude, double dc,
              double offset)
	: offset_(int(::rint(offset * gatesPerCycle))),
	  gatesPerCycle_(gatesPerCycle), amplitude_(amplitude),
	  dc_(dc), gateChange_(2.0 * M_PI / gatesPerCycle),
	  values_(), gate_(0)
	{
	    values_.reserve(gatesPerCycle);
	    if (offset_ < 0) offset_ += gatesPerCycle;
	}

    double getAmplitude() const { return amplitude_; }

    double getValue()
	{
	    if (values_.empty()) {
		int counter = offset_;
		for (int index = 0; index < gatesPerCycle_; ++index) {
		    if (counter == gatesPerCycle_)
			counter = 0;
		    values_.push_back(::sin(counter++ * gateChange_) *
                                      scaledAmplitude_ +
                                      scaledDC_);
		}
	    }

	    if (gate_ == gatesPerCycle_)
		gate_ = 0;
	    return values_[gate_++];
	}

    operator double() { return getValue(); }

    void setAmplitudeSum(double amplitudeSum)
	{
	    scaledAmplitude_ = amplitude_ / amplitudeSum;
	    scaledDC_ = dc_ / amplitudeSum;
	}

private:
    int offset_;
    int gatesPerCycle_;
    double amplitude_;
    double scaledAmplitude_;
    double dc_;
    double scaledDC_;
    double gateChange_;
    std::vector<double> values_;
    int gate_;
};

struct SetAmplitudeSum
{
    double sum_;
    SetAmplitudeSum(double sum) : sum_(sum) {}
    void operator()(Generator& generator)
	{ generator.setAmplitudeSum(sum_); }
};

class GeneratorCollection
{
public:
    GeneratorCollection() : container_(), sum_(0.0) {}

    void add(const Generator& generator)
	{
	    container_.push_back(generator);
	    updateAmplitudeSum(generator.getAmplitude());
	}

    double getValue()
	{ return std::accumulate(container_.begin(), container_.end(), 0.0); }

private:
    void updateAmplitudeSum(double amplitude)
	{
	    sum_ += amplitude;
	    std::for_each(container_.begin(), container_.end(),
                          SetAmplitudeSum(sum_));
	}

    std::vector<Generator> container_;
    double sum_;
};

int
main(int argc, char** argv)
{
    size_t numGates = Messages::RadarConfig::GetGateCountMax();
    size_t numRadials = Messages::RadarConfig::GetRadialCount();
    size_t numScans = 1;
    double minRange = Messages::RadarConfig::GetRangeMin_deprecated();
    double maxRange = Messages::RadarConfig::GetRangeMax();
    double minPower = -8192;
    double maxPower =  8192;
    double frequency = 360.0;

    Utils::CmdLineArgs cla(argc, argv, about, options, sizeof(options),
                           args, sizeof(args));
    std::string value;
    bool useComplex = cla.hasOpt("complex");

    if (cla.hasOpt("frequency", value))
	if (! (value >> frequency) || frequency <= 0.0)
	    cla.usage("invalid 'frequency' value");

    if (cla.hasOpt("gates", value))
	if (! (value >> numGates) || numGates < 1)
	    cla.usage("invalid 'samples' value");

    if (cla.hasOpt("minrange", value))
	if (! (value >> minRange) || minRange < 0.0)
	    cla.usage("invalid 'minrange' value");

    if (cla.hasOpt("maxrange", value))
	if (! (value >> maxRange) || maxRange < 1.0)
	    cla.usage("invalid 'maxrange' value");

    if (minRange >= maxRange)
	cla.usage("min range must be less than max range");

    if (cla.hasOpt("minpower", value))
	if (! (value >> minPower) || minPower < 0.0)
	    cla.usage("invalid 'minpower' value");

    if (cla.hasOpt("maxpower", value))
	if (! (value >> maxPower) || maxPower < 1.0)
	    cla.usage("invalid 'maxpower' value");

    if (minPower >= maxPower)
	cla.usage("min power must be less than max power");

    if (cla.hasOpt("radials", value))
	if (! (value >> numRadials) || numRadials < 1)
	    cla.usage("invalid 'radials' value");

    if (cla.hasOpt("scans", value))
	if (! (value >> numScans) || numScans < 1)
	    cla.usage("invalid 'scans' value");

    double priRate = 1.0 / frequency;
    double gateRate = priRate / double(numGates);
    double beamWidth = M_PI * 2.0 / double(numRadials);

    std::clog << "numRadials: " << numRadials
	      << " beamWidth: " << beamWidth
	      << " (" << Utils::radiansToDegrees(beamWidth) << " deg.)\n";
    std::clog << "numGates: " << numGates
	      << " minRange: " << minRange
	      << " maxRange: " << maxRange << '\n';
    std::clog << "minPower: " << minPower
	      << " maxPower: " << maxPower << '\n';
    std::clog << "priRate: " << priRate
	      << " frequency: " << frequency
	      << std::endl;

    GeneratorCollection generators;

    size_t index = 1;
    while (cla.hasArg(index++, value)) {
	std::istringstream is(value);
	double cycle, amplitude, dc, offset;
	char slash;
	if (! (is >> cycle))
	    cla.usage("invalid CYCLE generator component");
	if (! (is >> slash) || slash != '/')
	    cla.usage("invalid generator specification");
	if (! (is >> amplitude) || amplitude <= 0.0 || amplitude > 1.0)
	    cla.usage("invalid AMP generator component");
	if (! (is >> slash) || slash != '/')
	    cla.usage("invalid generator specification");
	if (! (is >> dc) || fabs(dc) + amplitude > 1.0)
	    cla.usage("invalid DC generator component");
	if (! (is >> slash) || slash != '/')
	    cla.usage("invalid generator specification");
	if (! (is >> offset) || offset < -1.0 || offset > 1.0)
	    cla.usage("invalid OFFSET generator component");

	cycle = 1.0 / cycle;
	int gatesPerCycle = int(::rint(cycle / gateRate));

	std::clog << "generator spec - gatesPerCycle: " << gatesPerCycle
		  << " amp: " << amplitude << " DC: " << dc << " offset: "
		  << offset << std::endl;

	generators.add(Generator(gatesPerCycle, amplitude, dc, offset));
    }

    // Create a new FileWriter object, and then 'connect' it to the file path given on the command line -- this
    // opens the file.
    //
    IO::FileWriter::Ref writer(IO::FileWriter::Make());
    ACE_FILE_Addr address(cla.arg(0).c_str());
    ACE_FILE_Connector fd(writer->getDevice(), address);
    writer->getDevice().truncate(0);

    Time::TimeStamp clock(0.0);

    Messages::VMEDataMessage vme;
    vme.header.msgDesc = (Messages::VMEHeader::kIRIGValidMask |
                          Messages::VMEHeader::kAzimuthValidMask |
                          Messages::VMEHeader::kPRIValidMask);
    if (useComplex) {
	vme.header.msgDesc |= (Messages::VMEHeader::kPackedIQ << 16);
    }
    else {
	vme.header.msgDesc |= (Messages::VMEHeader::kPackedReal << 16);
    }

    vme.header.pri = 0;
    vme.header.timeStamp = 0;
    vme.header.irigTime = 0;
    vme.rangeMin = minRange;
    vme.rangeFactor = (maxRange - minRange) / numGates;

    // Generate the requested number of scans
    //
    double powerOffset = (maxPower + minPower) / 2.0;
    double powerConversion = maxPower - powerOffset;
    std::clog << "powerOffset: " << powerOffset << " powerConversion: "
	      << powerConversion << std::endl;

    double minAmp = 10.0;
    double maxAmp = -10.0;

    for (size_t scan = 0; scan < numScans; ++scan) {

	// Generate the requested number of radials
	//
	for (size_t radial = 0; radial < numRadials; ++radial) {
	    ++vme.header.pri;
	    vme.header.azimuth =
		uint32_t(
		    ::rint(radial * beamWidth / (M_PI * 2.0) *
                           Messages::RadarConfig::GetShaftEncodingMax()));
	    vme.header.irigTime += priRate;
	    Messages::Video::Ref msg(Messages::Video::Make("sines", vme,
                                                           useComplex ?
                                                           numGates * 2 :
                                                           numGates));

	    msg->setCreatedTimeStamp(clock);
	    clock += priRate;

	    for (size_t gate = 0; gate < numGates; ++gate) {
		double amplitude = generators.getValue();
		if (amplitude > maxAmp) maxAmp = amplitude;
		else if (amplitude < minAmp) minAmp = amplitude;
		msg->push_back(int(::rint(amplitude * powerConversion +
                                          powerOffset)));
		if (useComplex)
		    msg->push_back(0);
	    }

	    IO::MessageManager mgr(msg);
	    writer->write(mgr);
	}
    }

    std::clog << "minAmp: " << minAmp << " maxAmp: " << maxAmp << std::endl;
    return 0;
}
