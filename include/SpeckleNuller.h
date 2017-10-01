#include <opencv2/opencv.hpp>
#include <iostream>
#include "params.h"
#include "ImageGrabber.h"
#include "Speckle.h"
#include "P3KCom.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>

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
        cv::Mat image, probeFlatmap, nullingFlatmap;
        ImageGrabber imgGrabber;
        std::vector<Speckle> specklesList;
        boost::property_tree::ptree cfgParams;
        P3KCom *p3k;
        bool verbose;
        //Speckle *speckleList;

    public:
        SpeckleNuller(boost::property_tree::ptree &ptree, bool vbose=false);
        std::vector<ImgPt> detectSpeckles();
        void updateImage(uint64_t timestamp);
        void createSpeckleObjects(std::vector<ImgPt> &imgPts);
        void measureSpeckleProbeIntensities(int phaseInd);
        void calculateFinalPhases();
        void generateProbeFlatmap(std::vector<int> &phaseInds);
        void generateProbeFlatmap(int phaseInd);
        void generateNullingFlatmap(double gain=DEFAULTGAIN);
        void updateCurFlatmap();
        void generateSimProbeSpeckles(int phaseInd);
        void generateSimFinalSpeckles(double gain=DEFAULTGAIN);
        void loadProbeSpeckles();
        void loadNullingSpeckles();
        void clearSpeckleObjects();
        //void applyCorrection(

};
#endif
