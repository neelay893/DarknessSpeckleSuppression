#include <opencv2/opencv.hpp>
#include <iostream>
#include "params.h"

#ifndef SPECKLENULLER_H
#define SPECKLENULLER_H
using namespace cv;

struct ImgPt
{
    Point2i coordinates;
    int intensity;
    bool operator<(ImgPt &rPt){return intensity < rPt.intensity;}

};

class SpeckleNuller
{
    private: 
        Mat image, curFlatmap;
        //Speckle *speckleList;

    public:
        SpeckleNuller();
        void detectSpeckles();
        void updateImage();
        //void applyCorrection(

};
#endif
