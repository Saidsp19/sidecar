#include "ace/FILE_Connector.h"
#include "ace/Reactor.h"
#include "ace/Stream.h"

#include "IO/FileWriterTask.h"
#include "IO/MessageManager.h"
#include "IO/Readers.h"

#include "Logger/Log.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"
#include "Utils/FilePath.h"

#include "ImageSegmentation.h"
#include "SegmentedTargetImage.h"

#include "Centroid.h"
#include "VideoStorage.h"
#include "boost/numeric/ublas/io.hpp"
#include "boost/numeric/ublas/matrix.hpp"
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace SideCar::IO;
using namespace SideCar::Messages;
using namespace std;

int im1[] = {0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1};

int im2[] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1};

int pris[] = {
    //  0  1  2  3  4  5  6  7  8  9  10
    0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0,  // 0
    0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0,  // 1
    0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1,  // 2
    0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0,  // 3
    0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0,  // 4
    0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0,  // 5
    0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0,  // 6
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,  // 7
    0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0,  // 8
    0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0,  // 9
    0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0,  // 10
    0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0,  // 11
    0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0,  // 12
    0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // 13

void
readCSV(ifstream& ifs, vector<BinaryScanLineVector>& vec)
{
    BinaryScanLineVector* row;

    std::string line;
    while (std::getline(ifs, line)) {
        row = new BinaryScanLineVector(new vector<BINARYDATA>);
        std::string tok1;
        std::istringstream iss(line);
        while (std::getline(iss, tok1, ',')) { row->push_back((BINARYDATA)atof(tok1.c_str())); }
        vec.push_back(*row);
        delete row;
    }
}

void
readCSV(ifstream& ifs, vector<VideoScanLineVector>& vec)
{
    VideoScanLineVector row;

    std::string line;
    while (std::getline(ifs, line)) {
        std::string tok1;
        std::istringstream iss(line);
        while (std::getline(iss, tok1, ',')) { row.push_back((VIDEODATA)atof(tok1.c_str())); }
        vec.push_back(row);
        row.clear();
    }
}

int
main(int argc, const char* argv[])
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);
    Logger::Log::Root().debug() << "this is a debug message" << std::endl;

    /*
      MaskedTargetImagePtr mti1(new MaskedTargetImage(Logger::Log::Root()));
      MaskedTargetImagePtr mti2(new MaskedTargetImage(Logger::Log::Root()));
      VideoScanLine vid;

      mti1->AddDataToLastRow(2,5);
      mti1->AddDataToLastRow(8,15);
      mti1->FinalizeRow(vid, 0);
      mti1->AddDataToLastRow(5,8);
      mti1->FinalizeRow(vid, 1);
      mti1->AddDataToLastRow(2,5);
      mti1->AddDataToLastRow(8,15);
      mti1->FinalizeRow(vid, 2);
      mti1->Dump();

      //  mti2->AddDataToLastRow(18,18);
      //mti2->FinalizeRow(vid,0);
      mti2->AddDataToLastRow(18,18);
      mti2->FinalizeRow(vid,2);
      mti2->AddDataToLastRow(18,18);
      //mti2->FinalizeRow(vid,2);
      mti2->Dump();

      RegionVector rv;
      rv.push_back(mti1);
      rv.push_back(mti2);
      MaskedTargetImagePtr mti3 = MaskedTargetImage::Merge(rv, 15,18, Logger::Log::Root());
      mti3->Dump();
    */

    /*
      int rows = 14;
      if (argc == 2) {
      rows = atoi(argv[1]);
      cout << "rows=" << rows << endl;
      }
      int cols = 11;
      BinaryScanLine bl;
      VideoScanLine vl;
      ImageSegmentation is(Logger::Log::Root());
      TargetSize discardSize;
      //discardSize.priCount = 6;
      //discardSize.priValid = true;
      for (int r = 0; r < rows; r++) {
      for (int c = 0; c < cols; c++) {
      bl.push_back(pris[r*cols + c]);
      vl.push_back(pris[r*cols + c]);
      }
      is.AppendScanLine(bl, r, discardSize);
      bl.clear();
      vl.clear();
      printf("\n");
      is.Dump();
      }
      printf("done\n");

      while (!is.IsClosedRegionsEmpty()) {
      MaskedTargetImagePtr target = is.PopClosedRegion();
      BinaryTargetImagePtr mask = target->MakeBinaryTargetImage();
      mask->Dump();
      }
    */

    ifstream mask("/export/home/data/need_centroiding/mask.csv");
    ifstream vid("/export/home/data/need_centroiding/vid.csv");

    if (!mask || !vid) { std::cout << "unable to find files" << std::endl; }

    vector<BinaryScanLineVector> binaryImage;
    vector<VideoScanLineVector> videoImage;

    ofstream out;
    char filename[256];

    readCSV(mask, binaryImage);
    readCSV(vid, videoImage);

    mask.close();
    vid.close();

    cout << "done reading in files: " << binaryImage.size() << " " << videoImage.size() << endl;

    ImageSegmentation is(Logger::Log::Root());
    VideoStorage vidHist(Logger::Log::Root());
    TargetSize discardSize;
    int targetCount = 0;
    for (int r = 0; r <= binaryImage.size(); r++) {
        if (r < binaryImage.size()) {
            is.AppendScanLine(binaryImage[r], r, discardSize);
            vidHist.Append(videoImage[r]);
        } else {
            // force an empty line to close any targets
            vector<BINARYDATA> temp;
            is.AppendScanLine(BinaryScanLineVector(&temp), r, discardSize);
        }

        while (!is.IsClosedTargetsEmpty()) {
            cout << "image closed" << endl;
            SegmentedTargetImagePtr target = is.PopClosedTarget();
            BinaryTargetImagePtr mask = target->MakeBinaryTargetImage();
            VideoTargetImagePtr video = vidHist.GetWindow(target->GetSize());
            video->SetAzimuthData(mask->GetAzimuthData());
            /*
            //create a centroider
            Centroid centroider (mask, video, Logger::Log::Root());

            centroider.Process();

            sprintf(filename, "target%i_subtargs.csv", targetCount);
            ofstream targsout;
            targsout.open(filename, ios::trunc | ios::binary);
            if (!targsout) { cout << "unable to open file " << filename << endl; abort(); }
            while (!centroider.IsClosedRegionsEmpty()) {
            TargetPosition cenPos = centroider.PopClosedRegionPosition();
            targsout << cenPos.range << ", " << cenPos.az << std::endl;
            }
            targsout.close();
            */

            mask->Dump();
            video->Dump();

            VideoTargetImagePtr x_grad;
            VideoTargetImagePtr y_grad;
            video->MaskGradient(mask, x_grad, y_grad);
            BinaryTargetImagePtr peaks_x = x_grad->IdentifyPeaksX();
            BinaryTargetImagePtr peaks_y = y_grad->IdentifyPeaksY();
            BinaryTargetImagePtr targetPeaks = (*peaks_x) && (*peaks_y);

            ImageSegmentation tpis(Logger::Log::Root());
            int rows = targetPeaks->GetRows();
            int cols = targetPeaks->GetCols();
            BINARYDATA* data = targetPeaks->GetDataRef();
            AzimuthDataPtr az = mask->GetAzimuthData();

            sprintf(filename, "target%i_subtargs.csv", targetCount);
            out.open(filename, ios::trunc | ios::binary);
            if (!out) {
                cout << "unable to open file " << filename << endl;
                abort();
            }
            for (int r = 0; r < rows; r++) {
                tpis.AppendScanLine(BinaryScanLineArray(data + r * cols, cols), (*az)[r]);
                while (!tpis.IsClosedTargetsEmpty()) {
                    SegmentedTargetImagePtr peakOfTarget = tpis.PopClosedTarget();
                    TargetSize tpsize = peakOfTarget->GetSize();
                    TargetPosition tppos = tpsize.Center();
                    cout << "   sub-target at rangebin= " << tppos.range << " az=" << tppos.az << endl;
                    out << tppos.range << "," << tppos.az << "," << std::endl;
                }
            }
            out.close();

            sprintf(filename, "target%i_xgrad.csv", targetCount);
            out.open(filename, ios::trunc | ios::binary);
            if (!out) {
                cout << "unable to open file " << filename << endl;
                abort();
            }
            x_grad->DumpCSV(out);
            out.close();

            sprintf(filename, "target%i_ygrad.csv", targetCount);
            out.open(filename, ios::trunc | ios::binary);
            if (!out) {
                cout << "unable to open file " << filename << endl;
                abort();
            }
            y_grad->DumpCSV(out);
            out.close();

            sprintf(filename, "target%i_xpeaks.csv", targetCount);
            out.open(filename, ios::trunc | ios::binary);
            if (!out) {
                cout << "unable to open file " << filename << endl;
                abort();
            }
            peaks_x->DumpCSV(out);
            out.close();

            sprintf(filename, "target%i_ypeaks.csv", targetCount);
            out.open(filename, ios::trunc | ios::binary);
            if (!out) {
                cout << "unable to open file " << filename << endl;
                abort();
            }
            peaks_y->DumpCSV(out);
            out.close();

            sprintf(filename, "target%i_targetPeaks.csv", targetCount);
            out.open(filename, ios::trunc | ios::binary);
            if (!out) {
                cout << "unable to open file " << filename << endl;
                abort();
            }
            targetPeaks->DumpCSV(out);
            out.close();

            targetCount++;
        }

        vidHist.SetDepth(is.GetMaxRowDepth());
    }
    is.Dump();
    cout << "done, the video depth is " << vidHist.GetDepth() << endl;
    cout << std::numeric_limits<short>::max() << endl;
    cout << std::numeric_limits<short>::min() << endl;

    int x = -32769;
    VIDEODATA test = (VIDEODATA)std::min<int>(std::max<int>(x, (int)std::numeric_limits<VIDEODATA>::min()),
                                              (int)std::numeric_limits<VIDEODATA>::max());
    cout << "x=" << x << ", test=" << test << endl;
}
