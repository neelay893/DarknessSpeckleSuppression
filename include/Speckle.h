#include <opencv2/opencv.hpp>
#include <iostream>

#ifndef SPECKLE_H
#define SPECKLE_H
class Speckle
{
    private:
        double phaseList[NPHASES];
        unsigned short intensities[NPHASES];
        double finalPhase;
        double nullingIntensity;
        cv::Point2i coordinates;
        cv::Mat apertureMask;

    public:
        Speckle();
        cv::Mat &generatePhaseFlatmap(int phaseInd); //think about storing after computation, possibly computing in advance
        cv::Mat &generateFinalFlatmap();
        cv::Mat incrementPhaseIntensity(int phaseInd, unsigned short intensity);
        unsigned short getSpeckleIntensity(Mat &image);

};
#endif
