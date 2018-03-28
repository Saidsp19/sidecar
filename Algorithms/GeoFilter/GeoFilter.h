#ifndef SIDECAR_ALGORITHMS_GEOFILTER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_GEOFILTER_H

#include "boost/scoped_ptr.hpp"

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm GeoFilter. Please describe what the algorithm does, in layman's terms and,
    if possible, mathematical terms.
*/
class GeoFilter : public Algorithm {
    using Super = Algorithm;

public:
    enum InfoSlots { kEnabled = ControllerStatus::kNumSlots, kActiveFilterCount, kConfigPath, kNumSlots };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    GeoFilter(Controller& controller, Logger::Log& log);

private:
    bool startup();

    bool shutdown();

    bool reset();

    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots(IO::StatusBase& status);

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processInput(const Messages::Video::Ref& msg);

    bool loadConfig();

    bool loadConfigFile(const std::string& path);

    void loadNotification(const Parameter::NotificationValue& value);

    // Add attributes here
    //
    Parameter::BoolValue::Ref enabled_;
    Parameter::ReadPathValue::Ref configPath_;
    Parameter::NotificationValue::Ref load_;

    struct Private;
    boost::scoped_ptr<Private> p_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
