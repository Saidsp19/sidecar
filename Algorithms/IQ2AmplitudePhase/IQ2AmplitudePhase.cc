#include "IQ2AmplitudePhase.h"
#include "Messages/RadarConfig.h"
using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

#include "Utils/VsipVector.h"
#include <vsip/math.hpp>

IQ2AmplitudePhase::IQ2AmplitudePhase(Controller& controller, Logger::Log &log)
    : Algorithm(controller, log),
      param_k(Parameter::DoubleValue::Make("k", 2)), old(0), older(0)
{
}

bool IQ2AmplitudePhase::startup()
{
    registerInput<IQ2AmplitudePhase,Video>(0);
    registerInput<IQ2AmplitudePhase,Video>(1);

    old=0;
    older=0;

    return registerParameter(param_k);
}

// Calculate Y=X1-X2
bool
IQ2AmplitudePhase::process(Messages::Video::Ref msg, uint port)
{
    // Buffer both channels
    switch(slot)
    {
    case 0:
        I.push_back(msg);
        break;
    case 1:
        Q.push_back(msg);
        break;
    default:
        // unknown channel
        return false;
    }

    // Loop until one of the channels is empty
    while(!I.empty() && !Q.empty())
    {
        int n1=I.front().getSequenceNumber();
        int n2=Q.front().getSequenceNumber();

        if(n1==n2)
        {
            Messages::Video::Ref amp("IQ2AmplitudePhase (amplitude)", I.front());
            Messages::Video::Ref phase("IQ2AmplitudePhase (phase)", I.front());
            amp.resize(I.front().size(), 0);
            phase.resize(I.front().size(), 0);

            VsipVector<Video> x(I.front());
            VsipVector<Video> y(Q.front());
            VsipVector<Video> vAmp(*amp);
            VsipVector<Video> vPhase(*phase);
            x.admit(true);
            y.admit(true);
            vAmp.admit(false);
            vPhase.admit(false);

            vAmp.v=hypot(x.v, y.v);
            vPhase.v=atan2(y.v, x.v);

            x.release(false);
            y.release(false);
            vAmp.release(true);
            vPhase.release(true);

            I.pop_front();
            Q.pop_front();

            if(!send(amp, 0))
                return false;

            if(!send(phase, 1))
                return false;
        }
        else if(n1<n2)
        {
            I.pop_front();
        }
        else
        {
            Q.pop_front();
        }
    }

    return true;
}

// DLL support
//
extern "C" ACE_Svc_Export Algorithm*
IQ2AmplitudePhaseMake(Controller& controller, Logger::Log& log)
{
    return new IQ2AmplitudePhase(controller, log);
}
