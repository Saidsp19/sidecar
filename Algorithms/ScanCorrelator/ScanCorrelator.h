#ifndef SIDECAR_ALGORITHMS_SCAN_CORRELATOR_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_SCAN_CORRELATOR_H

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Messages/Extraction.h"
#include "Parameter/Parameter.h"

#include <list>
#include <vector>
#include <cmath>

namespace SideCar {
namespace Algorithms {


/**
   \ingroup Algorithms
*/
class ScanCorrelator : public Algorithm
{
public:
    // Algorithm interface
    //
    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    ScanCorrelator(Controller& controller, Logger::Log& log);

    /** Register runtime parameters with the controller.

        \return true if successful, false otherwise
    */
    bool startup();

    bool reset();

private:

    void init();

    /**

       \param mgr object containing the encoded or native data to process

       \return true if successful, false otherwise
    */
    bool process(const Messages::Extractions::Ref& msg);

    using time_t = long;
    using Extraction = SideCar::Messages::Extraction;

    struct Data
    {
        time_t time;
        SideCar::Messages::Extraction extraction;
    };

    using Entry = std::list<Data>;
    std::vector<std::vector<Entry> > buffer; // [x][y]
    size_t indexOffset;

    void corrCell(Data &, Entry& candidates);
    void corr(Extraction &ext, time_t t);

    double rMax; 
    size_t numBins;

    Parameter::DoubleValue::Ref param_searchRadius;

    void on_searchRadius_changed(const Parameter::DoubleValue &);

    Parameter::IntValue::Ref param_scanTime;

    void on_scanTime_changed(const Parameter::IntValue &);

    Parameter::IntValue::Ref param_numScans;

    float searchRadius; // the maximum gating size for correlation to occur
    float searchRadius2; // = searchRadius^2

    time_t t_new, t_old, t_veryold;
};

}} // namespaces



/** \file
 */

#endif
