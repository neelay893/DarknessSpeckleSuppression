#include <opencv2/opencv.hpp>
#include <iostream>
#include "params.h"
#include "ImageGrabberSim.h"
#include "Speckle.h"

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
        std::vector<Speckle> specklesList;
        //Speckle *speckleList;

    public:
        SpeckleNuller();
        std::vector<ImgPt> detectSpeckles();
        void updateImage();
        void createSpeckles(std::vector<ImgPt> &imgPts);
        //void applyCorrection(

};
#endif
