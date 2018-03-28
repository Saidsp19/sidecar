#include "boost/bind.hpp"
#include <cmath>

#include "Messages/RadarConfig.h"
#include "TrackerReceive.h"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

TrackerReceive::TrackerReceive(Controller& controller, Logger::Log& log) : Algorithm(controller, log)
{
}

bool
TrackerReceive::startup()
{
    // Spawn the receive thread
    int started = ACE_Thread_Manager::instance()->spawn(TrackerReceive::threaded_receiver,
                                                        static_cast<void*>(this), // null argument pointer
                                                        THR_DETACHED | THR_SCOPE_SYSTEM);

    if (started < 0) return false;

    return true;
}

void*
TrackerReceive::threaded_receiver(void* arg)
{
    TrackerReceive* alg = static_cast<TrackerReceive*>(arg);
    Extractions::Ref out;

    Server rp_serv(12421);
    vector<int> rp_sockets;
    int maxd;

    rp_serv.Bind();
    rp_serv.Listen();

    rp_sockets.push_back(rp_serv.get_socketd());
    maxd = rp_sockets.front();

    fd_set fds;
    timeval tv;
    vector<int>::iterator iter;

    while (true) {
        FD_ZERO(&fds);
        for (iter = rp_sockets.begin(); iter != rp_sockets.end(); iter++) FD_SET(*iter, &fds);

        // listen for new connections or incoming messages
        tv.tv_sec = 0;
        tv.tv_usec = 500000;
        int sel_status = select(maxd + 1, &fds, NULL, NULL, &tv);
        if (!sel_status)
            continue;
        else if (sel_status < 0) {
            cerr << "Select error!" << endl;
            // error handle
        }

        // if new tracker is trying to connect
        if (FD_ISSET(rp_sockets.front(), &fds)) {
            int sd = rp_serv.Accept();
            if (sd > maxd) maxd = sd;
            rp_sockets.push_back(sd);
            cout << "New tracker connected at socket " << sd << endl;
            continue;
        }

        IADSMessage iads_msg;
        TrackMessage track_msg;
        int msg_len;

        for (iter = rp_sockets.begin() + 1; iter != rp_sockets.end(); iter++) {
            if (FD_ISSET(*iter, &fds)) {
                msg_len = rp_serv.Recv(*iter, &iads_msg);

                // A tracker has been disconnected
                if (!msg_len) {
                    cout << "Tracker has been disconnected at socket " << *iter;
                    cout << endl;
                    rp_serv.Close(*iter);
                    rp_sockets.erase(iter);
                    break;
                }
            }
            if (iads_msg.header.type = TRACK) {
                track_msg = TrackMessage(iads_msg);

                Track* trackInfo = track_msg.get_tracks();
                out = Extractions::Make("TrackerReceive");

                for (size_t curTrackNum = 0; curTrackNum < track_msg.num_tracks(); curTrackNum++) {
                    uint32_t npt = trackInfo->npts;
                    uint32_t track_id = trackInfo->track_id;

                    double residue = trackInfo->residue;
                    double noise[2][2];

                    for (int cc = 0; cc < 2; cc++)
                        for (int dd = 0; dd < 2; dd++) noise[cc][dd] = trackInfo->noise[cc][dd];

                    double cov[9][9];

                    for (int cc = 0; cc < 9; cc++)
                        for (int dd = 0; dd < 9; dd++) cov[cc][dd] = trackInfo->cov[cc][dd];

                    double* time = trackInfo->time;

                    double* lat[3];
                    double* lon[3];
                    double* alt[3];

                    for (int cc = 0; cc < 3; cc++) {
                        lat[cc] = trackInfo->lat[cc];
                        lon[cc] = trackInfo->lon[cc];
                        alt[cc] = trackInfo->alt[cc];
                    }

                    float range = degreesLat2Meters(lat[0][0]) * 0.001;
                    float azimuth = lon[0][0] * M_PI / 180;

                    out->push_back(Extraction(range, azimuth));

                    cout << track_id << " at (" << range << ", " << azimuth << ")" << endl;

                    trackInfo++;
                }
                assert(alg->send(out));
            } else if (iads_msg.header.type = RMTRACK) {
                // we have to decide what we want to do
                // with this
            }
        }
    }

    rp_serv.Close();

    return 0;
}

// DLL support
//
extern "C" ACE_Svc_Export Algorithm* TrackerReceiveMake(Controller& controller, Logger::Log& log);

Algorithm*
TrackerReceiveMake(Controller& controller, Logger::Log& log)
{
    return new TrackerReceive(controller, log);
}
