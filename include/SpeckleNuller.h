#include <opencv2/opencv.hpp>
#include <iostream>
#include "params.h"
#include <ImageGrabberSim.h>

#ifndef SPECKLENULLER_H
#define SPECKLENULLER_H

struct ImgPt
{
    cv::Point2i coordinates;
    unsigned short intensity;
    bool operator<(ImgPt &rPt){return intensity > rPt.intensity;}

};

class SpeckleNuller
{
    private: 
        cv::Mat image, curFlatmap;
        ImageGrabberSim imgGrabber;
        //Speckle *speckleList;

    public:
        SpeckleNuller();
        void detectSpeckles();
        void updateImage();
        //void applyCorrection(

};
#endif
