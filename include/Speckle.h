#include <opencv2/opencv.hpp>
#include <iostream>
#include <params.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <boost/property_tree/ptree.hpp>
#include "dmTools.h"
#include "simInterfaceTools.h"

#ifndef SPECKLE_H
#define SPECKLE_H
class Speckle
{
    private:
        double phaseList[NPHASES];
        double gainList[NGAINS];
        unsigned int phaseIntensities[NPHASES];
        unsigned int gainIntensities[NGAINS];
        unsigned int initialIntensity;
        double finalPhase;
        double finalGain;
        double nullingIntensity;
        cv::Point2i coordinates;
        cv::Point2d kvecs;
        cv::Mat apertureMask;
        boost::property_tree::ptree cfgParams;
        void computeSpecklePhase();

    public:
        Speckle(cv::Point2i &pt, boost::property_tree::ptree &ptree);
        void incrementPhaseIntensity(int phaseInd, unsigned int intensity);
        void incrementGainIntensity(int gainInd, unsigned int intensity);
        void setInitialIntensity(unsigned int intensity);
        unsigned int measureSpeckleIntensity(cv::Mat &image);
        void calculateFinalPhase();
        void calculateFinalGain();
        double getFinalGain();
        double getFinalPhase();

        cv::Mat getProbeSpeckleFlatmap(int phaseInd);
        cv::Mat getProbeGainSpeckleFlatmap(int gainInd);
        cv::Mat getFinalSpeckleFlatmap(double gain=DEFAULTGAIN);
        void generateSimProbeSpeckle(int phaseInd);
        void generateSimFinalSpeckle(double gain);

        cv::Point2i getCoordinates();

};
#endif
