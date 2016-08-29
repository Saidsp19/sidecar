#include <stdlib.h>
#include <iostream>
#include <string>
using std::cout;
using std::string;

#include <ace/FILE_Connector.h>

#include "IO/MessageManager.h"
using SideCar::IO::MessageManager;

#include "IO/Writers.h"
using SideCar::IO::FileWriter;

#include "Messages/Video.h"
using SideCar::Messages::Video;
using SideCar::Messages::VMEDataMessage;

int scanint(const char *str, const char *message, int def)
{
    char *end=0;
    errno=0;
    int val=int(strtol(str, &end, 10));

    if(errno || end==str)
    {
        errno=0;
        cout << message << str << "\n"
            << "- using " << def << " instead.\n";
        return def;
    }

    return val;
}

float scanfloat(const char *str, const char *message, float def)
{
    char *end=0;
    errno=0;
    float val=strtof(str, &end);

    if(errno || end==str)
    {
        errno=0;
        cout << message << str << "\n"
            << "- using " << def << " instead.\n";
        return def;
    }

    return val;
}

int main(const int argc, char **argv)
{
    if(argc<4 || ((argc-4)%3))
    {
        cout << "Usage: " << argv[0] << " <output file> <num pri's> <pri length> {<period> <amplitude> <complex phase>}\n";
        return 0;
    }

    string filename=argv[1];
    FileWriter output;
    ACE_FILE_Addr path(filename.c_str());
    ACE_FILE_Connector conn;
    if(conn.connect(output.getDevice(), path)==-1)
    {
        cout << "Failed to open " << filename << "\n";
        return 0;
    }

    int count=scanint(argv[2], "Invalid number of pri's: ", 1);
    int size=scanint(argv[3], "Invalid pri length: ", 1000);

    int signals=(argc-4)/3;
    int per[signals];
    float freq[signals];
    float amp[signals];
    float real[signals], imag[signals];
    for(int i=0; i<signals; i++)
    {
        per[i]=scanint(argv[3*i+4], "Invalid period: ", 1);
        freq[i]=2*M_PI/per[i];
        amp[i]=scanfloat(argv[3*i+5], "Invalid amplitude: ", 1);
        float phase=scanfloat(argv[3*i+6], "Invalid phase: ", 0);
        real[i]=sinf(phase);
        imag[i]=cosf(phase);
    }

    for(int i=0; i<count; i++)
    {
        VMEDataMessage vme;
        vme.header.timeStamp=i;
        vme.header.azimuth=i;
        vme.header.pri=i;
        vme.header.irigTime=i;
        vme.rangeMin=0;
        vme.rangeFactor=300.0/size;
        Video::Ref msg(Video::Make("PRIGen", vme, 0));
        msg->resize(size, 0);

        for(int j=0; j<size/2; j++)
        {
            float re=0, im=0;
            for(int k=0; k<signals; k++)
            {
                float x=amp[k]*cosf((j%per[k])*freq[k]);
                re+=x*real[k];
                im+=x*imag[k];
            }

            msg[2*j+0]=int16_t(re);
            msg[2*j+1]=int16_t(im);
        }

        MessageManager mgr(msg);
        output.write(mgr.getMessage());
    }
}

