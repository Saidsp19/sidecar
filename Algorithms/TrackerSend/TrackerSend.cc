#include "boost/bind.hpp"
#include <cmath>

#include "Messages/RadarConfig.h"
#include "TrackerSend.h"

// VIADS
#include <DetectionMessage.h>
#include <PPIInfoMessage.h>

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

TrackerSend::TrackerSend(Controller& controller, Logger::Log& log) :
    Algorithm(controller, log), client(0), ppiInfoSent(false)
{
    ;
}

bool
TrackerSend::startup()
{
    registerInput<TrackerSend, Messages::Extractions>();

    if (client) {
        client->Close();
        delete (client);
    }

    int port = 12321;
    char* address = "127.0.0.1";
    client = new Client(port, address);

    if (!client->Connect()) {
        delete (client);
        client = 0;
        return false;
    }

    ppiInfoSent = false;

    return true;
}

bool
TrackerSend::process(const Messages::Extractions::Ref& msg, uint port)
{
    static const int bytes_expected = 56;
    Extractions::iterator extraction;
    Extractions::const_iterator stop = msg->end();
    time_t time = msg->getCreatedTimeStamp().getSeconds();

    if (!ppiInfoSent) {
        PPIInfoMessage info(0, 27., // ID, period
                            0, 0,   // lat, lon
                            50, 10, // delta range, az
                            time);

        client->Send(&info);

        ppiInfoSent = true;
    }

    for (extraction = msg->begin(); extraction != stop; extraction++) {
        DetectionMessage dm(time, extraction->getRange(), extraction->getAzimuth() * 180 / M_PI);
        int bytes_sent = client->Send(&dm);
        if (bytes_sent != bytes_expected) return false;
    }

    return true;
}

// DLL support
//
extern "C" ACE_Svc_Export Algorithm*
TrackerSendMake(Controller& controller, Logger::Log& log)
{
    return new TrackerSend(controller, log);
}
