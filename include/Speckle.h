#include <opencv2/opencv.hpp>
#include <iostream>
#include <params.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include "dmTools.h"

#ifndef SPECKLE_H
#define SPECKLE_H
class Speckle
{
    private:
        double phaseList[NPHASES];
        unsigned short intensities[NPHASES];
        unsigned short initialIntensity;
        double finalPhase;
        double nullingIntensity;
        cv::Point2i coordinates, kvecs;
        cv::Mat apertureMask;
        void computeSpecklePhase();

    public:
        Speckle(cv::Point2i &pt, unsigned short intensity);
        cv::Mat &generatePhaseFlatmap(int phaseInd); //think about storing after computation, possibly computing in advance
        cv::Mat &generateFinalFlatmap();
        void incrementPhaseIntensity(int phaseInd, unsigned short intensity);
        unsigned short measureSpeckleIntensity(cv::Mat &image);

};
#endif
