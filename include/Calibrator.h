#include <opencv2/opencv.hpp>
#include <iostream>
#include "params.h"
#include "ImageGrabber.h"
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
        cv::Mat[4] centroidImages;
        double kx;
        double ky;
        vector<cv::Point2d> pos0, pos1, pos2, pos3; //Centroid measurements at different speckle positions

    public:
        cv::Point2d center; //Location of center PSF on image

}

#endif
