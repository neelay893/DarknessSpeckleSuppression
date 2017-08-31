#include <opencv2/opencv.hpp>
#include <iostream>
#include "params.h"
#include "ImageGrabber.h"
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
        cv::Mat image, curFlatmap, nextFlatmap;
        ImageGrabber imgGrabber;
        std::vector<Speckle> specklesList;
        //Speckle *speckleList;

    public:
        SpeckleNuller();
        std::vector<ImgPt> detectSpeckles();
        void updateImage();
        void createSpeckleObjects(std::vector<ImgPt> &imgPts);
        void generateProbeFlatmap(std::vector<int> &phaseInds);
        void generateProbeFlatmap(int phaseInd);
        //void applyCorrection(

};
#endif
