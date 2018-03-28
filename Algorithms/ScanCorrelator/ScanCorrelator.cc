#include "boost/bind.hpp"
#include <cmath>

#include "Logger/Log.h"
#include "Messages/RadarConfig.h"
#include "ScanCorrelator.h"
#include "ScanCorrelator_defaults.h"

using std::ceil;
using std::floor;

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Messages;
//#define SCAN_CORR_DEBUG
#ifdef SCAN_CORR_DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif

ScanCorrelator::ScanCorrelator(Controller& controller, Logger::Log& log) :
    Algorithm(controller, log),
    param_searchRadius(Parameter::DoubleValue::Make("searchRadius", "Search Radius", kDefaultSearchRadius)),
    param_scanTime(Parameter::IntValue::Make("scanTime", "Scan Time", time_t(RadarConfig::GetRotationDuration()))),
    param_numScans(Parameter::IntValue::Make("numScans", "Num scans to correlate over", kDefaultNumScans))
{
    param_searchRadius->connectChangedSignalTo(boost::bind(&ScanCorrelator::on_searchRadius_changed, this, _1));
    param_scanTime->connectChangedSignalTo(boost::bind(&ScanCorrelator::on_scanTime_changed, this, _1));

    on_scanTime_changed(*param_scanTime);
}

bool
ScanCorrelator::startup()
{
    registerProcessor<ScanCorrelator, Messages::Extractions>(&ScanCorrelator::process);
    if (!registerParameter(param_searchRadius)) return false;
    if (!registerParameter(param_scanTime)) return false;
    if (!registerParameter(param_numScans)) return false;
    reset();
    init();
    return Algorithm::startup();
}

bool
ScanCorrelator::reset()
{
    searchRadius = param_searchRadius->getValue();
    searchRadius2 = searchRadius * searchRadius;
    return true;
}

bool
ScanCorrelator::process(const Messages::Extractions::Ref& msg)
{
    int num_scans = param_numScans->getValue();

    Messages::Extractions::Ref result = Messages::Extractions::Make("ScanCorrelator", msg);

    Extractions::iterator index;
    Extractions::const_iterator stop = msg->end();
    time_t time = msg->getCreatedTimeStamp().getSeconds();

    for (index = msg->begin(); index != stop; index++) {
        corr(*index, time);

#ifdef SCAN_CORR_DEBUG
        cerr << "correlated? " << index->getCorrelated() << " at " << index->getX() << ", " << index->getY() << endl;
#endif

        if (index->getCorrelated() && (index->getNumCorrelations() >= num_scans)) result->push_back(*index);

        //        if(index->getCorrelations()>0)
        //            result->push_back(*index);
    }

    return send(result);
}

void
ScanCorrelator::init()
{
    double rMax = RadarConfig::GetRangeMax();

    // Initialize the buffer
    indexOffset = int(ceil(rMax / searchRadius)) + 1;
    numBins = 2 * indexOffset + 2; // +2 is to allow for less code in corr()

    buffer.resize(numBins);

    for (size_t i = 0; i < numBins; i++) {
        buffer[i].clear();
        buffer[i].resize(numBins);
    }
}

void
ScanCorrelator::corrCell(Data& d, Entry& candidates)
{
    static Logger::ProcLog log("corrCell", getLog());
    Extraction& ext = d.extraction;

#ifdef SCAN_CORR_DEBUG
    cerr << "Cell count: " << candidates.size() << endl;
#endif

    Entry::iterator ci;
    Entry::const_iterator stop = candidates.end();

    for (ci = candidates.begin(); ci != stop;) {
        time_t deltat = d.time - ci->time;

        // Ignore/discard old entries
        if (deltat > t_old) {
#ifdef SCAN_CORR_DEBUG
            cerr << "too old." << endl;
#endif

            if (deltat > t_veryold) {
#ifdef SCAN_CORR_DEBUG
                cerr << "very old, erase." << endl;
#endif

                ci = candidates.erase(ci);
            } else {
                ci++;
            }
            continue;
        }

        // Ignore new entries
        if (deltat < t_new) {
#ifdef SCAN_CORR_DEBUG
            cerr << "too new." << endl;
#endif

            ci++;
            continue;
        }

        const Extraction& other = ci->extraction;
        // Verify the exact cartesian distance
        float dx = ext.getX() - other.getX();
        float dy = ext.getY() - other.getY();

        float dist2 = dx * dx + dy * dy;
#ifdef SCAN_CORR_DEBUG
        cerr << "too far? ";
#endif

        if (dist2 < searchRadius2) {
#ifdef SCAN_CORR_DEBUG
            cerr << "no.  correlating." << endl;
#endif

            // ext.correlate(other, sqrt(dist2));
            ext.setCorrelated(TRUE);
            int num_scans = other.getNumCorrelations();
            num_scans++;
            ext.setNumCorrelations(num_scans);
            return;
        } else {
#ifdef SCAN_CORR_DEBUG
            cerr << "yes." << endl;
#endif
        }

        ci++;
    }
}

void
ScanCorrelator::corr(Extraction& ext, time_t time)
{
    // Determine which bin this extraction belongs to
    // (add +1) to shift the logical data buffer within the larger buffer
    int binX = int(floor(ext.getX() / searchRadius)) + indexOffset + 1;
    int binY = int(floor(ext.getY() / searchRadius)) + indexOffset + 1;

    ext.setCorrelated(FALSE);

    Data d = {time, ext};

#ifdef SCAN_CORR_DEBUG
    cerr << "Correlating extraction (" << ext.getX() << ", " << ext.getY() << ") into [" << binX << ", " << binY
         << "] numBins=" << numBins << endl;
#endif

    if (((binX - 1) < 0) || ((binX + 1) >= numBins) || ((binY - 1) < 0) || ((binY + 1) >= numBins)) {
#ifdef SCAN_CORR_DEBUG
        cerr << "Bad bin #, bailing" << endl;
#endif

        return;
    }

    // Perform the correlations
    // binX and binY are never = 0 or numBins - 1, so this code holds
    // for all possible cases
    corrCell(d, buffer[binX - 1][binY - 1]);
    corrCell(d, buffer[binX - 1][binY]);
    corrCell(d, buffer[binX - 1][binY + 1]);
    corrCell(d, buffer[binX][binY - 1]);
    corrCell(d, buffer[binX + 1][binY - 1]);
    corrCell(d, buffer[binX][binY]);
    corrCell(d, buffer[binX][binY + 1]);
    corrCell(d, buffer[binX + 1][binY]);
    corrCell(d, buffer[binX + 1][binY + 1]);

    if (!d.extraction.getCorrelated()) d.extraction.setNumCorrelations(0);

    // Add to buffer
    buffer[binX][binY].push_back(d);

    // set the result
    ext = d.extraction;
}

void
ScanCorrelator::on_searchRadius_changed(const Parameter::DoubleValue& x)
{
    searchRadius = param_searchRadius->getValue();
    searchRadius2 = searchRadius * searchRadius;
    init();
}

void
ScanCorrelator::on_scanTime_changed(const Parameter::IntValue& x)
{
    time_t scanRate = param_scanTime->getValue();
    t_new = scanRate / 2;
    t_old = scanRate + t_new;
    t_veryold = 2 * scanRate;
}

// DLL support
//
extern "C" ACE_Svc_Export Algorithm*
ScanCorrelatorMake(Controller& controller, Logger::Log& log)
{
    return new ScanCorrelator(controller, log);
}
