#include <opencv2/opencv.hpp>
#include <iostream>
#include "params.h"
#include "ImageGrabber.h"
#include "P3KCom.h"
#include "imageTools.h"
#include "dmTools.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>

#ifndef CALIBRATOR_H
#define CALIBRATOR_H
/**
* Keeps track of image center, determines DM parameters
* such as lambda/D
**/
class Calibrator
{
    private:
        //Attributes of speckles currently on the DM (or at least flatmap)
        std::vector<cv::Point2d> curKvecs; //speckle kvecs
        std::vector<cv::Point2d> curPosEstimates; //calculated position estimates
        std::vector<cv::Point2d> curNegEstimates; //pos est of opposit speckle
        std::vector<cv::Point2d> curTruePos; //speckle locations centroided from image
        std::vector<cv::Point2d> curTrueNeg;
        std::vector<bool> isValidPos; //true if speckle is actually on the array
        std::vector<bool> isValidNeg;
        std::vector<double> dmAmplitudes; //dm amplitudes of speckles on the DM

        boost::property_tree::ptree cfgParams;
        P3KCom *p3k;
        cv::Mat nullingFlatmap; //Apply satellite specks to nulling flatmap
        cv::Mat speckleFlatmap; //Flatmap for just static speckles

        cv::Mat image;
        cv::Mat filtImage;
        cv::Mat badPixMask;

    public:
        Calibrator(boost::property_tree::ptree &ptree, P3KCom *p);
        double lambdaOverD;
        cv::Point2d imgCenter;
        double dmAngle;

        void updateImage(cv::Mat img, bool addToCurImg=false);
        void updateBadPixMask(cv::Mat &bpMask);
        void filterImage(cv::Mat &img);

        cv::Point2d determineImgCenter();
        double determineDMAngle();

        void addCalSpeckle(cv::Point2d kvecs, double amplitude);
        void centroidCalSpeckles();
        cv::Point2d centroidPoint(cv::Point2d posEstimate);
        cv::Point2d calculatePositionEstimate(cv::Point2d kvec);

        void updateNullingFlatmap(cv::Mat &flatmap);
        void loadCalFlatmap();
        void unloadCalFlatmap();
        void clearCalSpeckles();

};

#endif
