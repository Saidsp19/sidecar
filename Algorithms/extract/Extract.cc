#include <algorithm> // for std::transform
#include <cmath>     // for floor
#include <deque>
#include <functional> // for std::bind* and std::mem_fun*

#include "IO/MessageManager.h"
#include "Logger/Log.h"
#include "Messages/BinaryVideo.h"
#include "Messages/Extraction.h"
#include "Messages/RadarConfig.h"
#include "Utils/Utils.h"

#include "Extract.h"

namespace SideCar {
namespace Algorithms {

/** Internal class used by the Extract class to keep track of a target extraction. Records the azimuth at which
    the target was first identified, and the bounding range gates the target might occupy.
*/
class Extract::Target {
public:
    using Ref = boost::shared_ptr<Target>;

    /** Factory class method that creates a new Target instance.

        \param gate sample index in the PRI message

        \param azimuth radar azimuth where first reported

        \return
    */
    static Ref Make(double irig, int gate, float azimuth)
    {
        Ref ref(new Target(irig, gate, azimuth));
        return ref;
    }

    /** Update the target with a new (larger) gate.

        \param gate new gate value
    */
    void update(double irig, int gate, float azimuth)
    {
        if (gate > gateMax_) gateMax_ = gate;
        irigEnd_ = irig;
        azimuthEnd_ = azimuth;
        valid_ = true;
    }

    /** Absorb the gate min/max values from another target and then delete the target.

        \param other target to absorb and delete

        \return pointer to self
    */
    void assimilate(const Target::Ref& other)
    {
        if (other->getGateMin() < gateMin_) gateMin_ = other->getGateMin();
        if (other->getGateMax() > gateMax_) gateMax_ = other->getGateMax();
        valid_ = true;
    }

    /** Obtain the value of our valid_ attribute, resetting it to false before returning.

        \return true if the target is still valid
    */
    bool testValidAndReset()
    {
        bool tmp = false;
        std::swap(tmp, valid_);
        return tmp;
    }

    double getIRIGTime() const { return (irigEnd_ + irigStart_) / 2.0; }

    /** Obtain the middle of the range of gate values.

        \return gate average
    */
    float getGate() const { return (gateMin_ + gateMax_) / 2.0; }

    /** Obtain the middle of the range of azimuth values

        \return azimuth average
    */
    float getAzimuth() const
    {
        double azimuthEnd = azimuthEnd_;
        if (azimuthEnd < azimuthStart_) azimuthEnd += 2 * M_PI;
        return Utils::normalizeRadians((azimuthStart_ + azimuthEnd) / 2.0);
    }

    /** Obtain the lowest range gate value

        \return gate minimum
    */
    int getGateMin() const { return gateMin_; }

    /** Obtain the highest range gate value

        \return gate maximum
    */
    int getGateMax() const { return gateMax_; }

private:
    /** Constructor for new target objects.

        \param gate first gate the target could be att

        \param azimuth azimuth of the PRI target was first seen in
    */
    Target(double irig, int gate, float azimuth) :
        irigStart_(irig), irigEnd_(irig), azimuthStart_(azimuth), azimuthEnd_(azimuth), gateMin_(gate), gateMax_(gate),
        valid_(true)
    {
    }

    double irigStart_;
    double irigEnd_;
    float azimuthStart_;
    float azimuthEnd_;
    int gateMin_;
    int gateMax_;
    bool valid_;
};

} // namespace Algorithms
} // namespace SideCar

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

Extract::Extract(Controller& controller, Logger::Log& log) : Algorithm(controller, log)
{
    ;
}

bool
Extract::startup()
{
    registerProcessor<Extract, Messages::BinaryVideo>(&Extract::process);
    return true;
}

bool
Extract::reset()
{
    pending_.clear();
    gateTargets_.clear();
    return true;
}

bool
Extract::process(const Messages::BinaryVideo::Ref& msg)
{
    static Logger::ProcLog log("process", getLog());

    // Make sure our declared vector is at least as big as the message vector since we will index into both at
    // the same time.
    //
    if (msg->size() > gateTargets_.size()) gateTargets_.resize(msg->size(), Target::Ref());

    // Step through all PRI range gates looking for OFF/ON and ON/OFF transitions to determine blobs. We know
    // the transition type based on the value of the target local variable.
    //
    Target::Ref target;
    const Messages::BinaryVideo::Container& data(msg->getData());
    for (size_t gate = 0; gate < data.size(); ++gate) {
        // See if there is a blob to detect
        //
        if (data[gate]) {
            // Is this the start of a new blob?
            //
            if (!target) {
                // Yes. If there was a target declaration from processing the last PRI, use it for this blob.
                // Otherwise, create a new target.
                //
                if (gateTargets_[gate]) {
                    target = gateTargets_[gate];
                } else {
                    target = Target::Make(msg->getIRIGTime(), gate, msg->getAzimuthStart());
                    pending_.push_back(target);
                }
            } else {
                // No, in the middle of a blob. Check if a diffferent target declaration from a previous PRI
                // exists. If so, use it and delete the current one.
                //
                if (gateTargets_[gate] && gateTargets_[gate] != target) {
                    TargetList::iterator pos = std::find(pending_.begin(), pending_.end(), target);
                    if (pos != pending_.end()) pending_.erase(pos);
                    gateTargets_[gate]->assimilate(target);
                    target = gateTargets_[gate];
                }
            }
        } else if (target) {
            // A blob has just ended, so we need to set its max gate. By now the blob should be associated with
            // one and only one target
            //
            target->update(msg->getIRIGTime(), gate - 1, msg->getAzimuthStart());
            target.reset();
        }
    }

    // Clear the gate target assignments. We will put them back in if necessary in the loop below, and we will
    // remember them for the next PRI message.
    //
    std::fill(gateTargets_.begin(), gateTargets_.end(), Target::Ref());

    // Visit each of the pending targets to see if it should be emitted as an extraction.
    //
    Extractions::Ref extractions;
    TargetList::iterator it(pending_.begin());
    TargetList::iterator pendingEnd(pending_.end());
    while (it != pendingEnd) {
        // Get the target at the current iterator position, but do not move the iterator yet since we may delete
        // the position and we don't want to invalidate the iterator if we do.
        //
        target = *it;

        // If the target was touched in the above gate processsing loop, reset the valid flag and assign the
        // target to the gates vector.
        //
        if (target->testValidAndReset()) {
            ++it;
            for (int gate = target->getGateMin(); gate <= target->getGateMax(); ++gate)
                if (data[gate]) gateTargets_[gate] = target;
        } else {
            // The target was not used in the above gate processsing loop, so we now know its final azimuth
            // value. Calculate the range/azimuth for an extraction entry.
            //
            double irig = target->getIRIGTime();
            float range = msg->getRangeAt(target->getGate());
            float azimuth = target->getAzimuth();

            LOGINFO << "IRIG: " << irig << " range: " << range << " azimuth: " << Utils::radiansToDegrees(azimuth)
                    << std::endl;

            if (!extractions) extractions = Extractions::Make("Extract", msg);

            extractions->push_back(Extraction(irig, range, azimuth, 0.0));

            // Finished with this target.
            //
            it = pending_.erase(it);
            pendingEnd = pending_.end();
        }
    }

    // Processing finished. Send off an extractions message if we did any extractions.
    //
    bool rc = true;
    if (extractions) rc = send(extractions);

    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

extern "C" ACE_Svc_Export Algorithm*
ExtractMake(Controller& controller, Logger::Log& log)
{
    return new Extract(controller, log);
}
