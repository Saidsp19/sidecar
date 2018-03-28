#include <limits>
#include <sstream>

#include "Logger/Log.h"
#include "Messages/TSPI.h"

#include "ABTracker.h"
#include "Track.h"

using namespace SideCar::Messages;
using namespace SideCar::Algorithms;
using namespace SideCar::Algorithms::ABTrackerUtils;

Logger::Log&
Track::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Algorithms.ABTracker.Track");
    return log_;
}

Track::Track(ABTracker& owner, uint32_t id, double when, const Geometry::Vector& pos) :
    owner_(owner), t0_(), initialPosition_(pos), id_(""), state_(kInitiating)
{
    Logger::ProcLog log("Track", Log());
    LOGINFO << std::endl;

    std::ostringstream os;
    os << id;
    id_ = os.str();
    t0_.when_ = when;
    t0_.position_ = pos;
    checkIfInitiated(when);
}

void
Track::checkIfInitiated(double when)
{
    static Logger::ProcLog log("checkIfInitiated", Log());
    LOGINFO << "when: " << when << std::endl;

    initiationTimeStamps_.push_back(when);

    // Prune any stale timestamps.
    //
    double maxDuration = owner_.getScaledMaxInitiationDuration();
    for (size_t index = 0; index < initiationTimeStamps_.size(); ++index) {
        double delta = when - initiationTimeStamps_[index];
        if (delta <= maxDuration) {
            if (index) {
                initiationTimeStamps_.erase(initiationTimeStamps_.begin(), initiationTimeStamps_.begin() + index - 1);
                break;
            }
        }
    }

    // Only become active if after pruning we have the required amount of hits.
    //
    if (initiationTimeStamps_.size() >= owner_.getInitiationCount()) {
        LOGINFO << "track " << id_ << " is now alive" << std::endl;
        initiationTimeStamps_.clear();
        state_ = kAlive;
    }
}

double
Track::getProximityTo(double when, const Geometry::Vector& pos)
{
    static Logger::ProcLog log("getProximityTo", Log());
    LOGINFO << id_ << " when: " << when << std::endl;

    // How much time has passed since the last update?
    //
    double deltaTime = when - t0_.when_;

    // If too much time has elapsed to initiate the track or to coast it, drop it.
    //
    if (state_ == kInitiating) {
        if (deltaTime >= owner_.getScaledMaxInitiationDuration()) {
            LOGINFO << id_ << " uninitiating - " << deltaTime << std::endl;
            state_ = kUninitiating;
        }
    } else if (state_ == kAlive) {
        if (deltaTime >= owner_.getScaledMaxCoastDuration()) {
            LOGINFO << id_ << " dropping - " << deltaTime << std::endl;
            state_ = kDropping;
        }
    }

    // If the track is not initiating or active, don't let it participate in plot association.
    //
    if (state_ == kDropping || state_ == kUninitiating) return std::numeric_limits<double>::max();

    // Predict track position assuming constant velocity. X1 = X0 + V0 * T
    //
    Geometry::Vector position = t0_.position_ + t0_.velocity_ * deltaTime;

    // Calculate proximity of given position to estimated position.
    //
    position -= pos;
    double value = position.getMagnitudeSquared();

    LOGDEBUG << id_ << " return: " << value << std::endl;
    return value;
}

void
Track::updatePosition(double when, const Geometry::Vector& pos)
{
    static Logger::ProcLog log("updatePosition", Log());
    static const double kMinDeltaTime = 1.0E-3;
    LOGINFO << id_ << " when: " << when << std::endl;

    // Calculate the delta time
    //
    double deltaTime = when - t0_.when_;
    if (deltaTime < kMinDeltaTime) {
        LOGDEBUG << "delta too small - " << deltaTime << std::endl;
        return;
    }

    t0_.when_ = when;

    // See if we can move into kAlive state
    //
    if (state_ == kInitiating) checkIfInitiated(when);

    // Calculate distance moved
    //
    Geometry::Vector distance = pos;
    Geometry::Vector error = pos;
    Geometry::Vector estimate = t0_.position_ + t0_.velocity_ * deltaTime;

    // Compute the difference between the measured position and the expected position
    //
    error -= estimate;
    distance -= t0_.position_;

    // If track is initiating, don't use the AB equation for the velocity since we don't have an initial
    // velocity estimate, and we want to quickly settle on one based on the first N associations.
    //
    if (state_ == kInitiating) {
        // Estimate velocity as distance traveled over time.
        //
        Geometry::Vector velocity = distance / deltaTime;

        if (initiationTimeStamps_.size() == 2) {
            // Just use the estimate.
            //
            t0_.velocity_ = velocity;
        } else {
            // Update velocity as an average of estimated and last value
            //
            t0_.velocity_ = (t0_.velocity_ + velocity) / 2.0;
        }
    } else {
        // Othewise, update using AB tracking-filter equation: V1 = V_Predicted + Beta * (X_Measured -
        //   X_Predicted) / T
        //
        t0_.velocity_ += owner_.getBeta() * error / deltaTime;
    }

    // Calculate estimated position: X1 = X_Predicted + Alpha * (X_Measured - X_Predicted)
    //
    t0_.position_ = estimate + owner_.getAlpha() * error;
}

bool
Track::emitPosition()
{
    if (state_ == kInitiating) return true;

    TSPI::Ref msg(TSPI::MakeRAE(owner_.getName(), id_, t0_.when_, t0_.position_.getMagnitude() * 1000.0,
                                t0_.position_.getDirection(), t0_.position_.getZ()));

    if (state_ == kDropping) msg->setDropping();

    return owner_.send(msg);
}

void
Track::drop()
{
    state_ = kDropping;
    emitPosition();
}
