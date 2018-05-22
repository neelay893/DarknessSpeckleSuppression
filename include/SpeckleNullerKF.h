#include <opencv2/opencv.hpp>
#include <iostream>
#include "params.h"
#include "ImageGrabber.h"
#include "Speckle.h"
#include "P3KCom.h"
#include "imageTools.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>

#ifndef SPECKLENULLER_H
#define SPECKLENULLER_H
/**
* Stores speckle coordinates and intensity. Mostly just for easy sorting in
* detectSpeckles method.
**/
struct ImgPt
{
    cv::Point2d coordinates;
    double intensity;
    bool operator<(ImgPt &rPt){return intensity > rPt.intensity;}

};

/**
* Implements the speckle nulling loop. Has methods for detecting, probing, and
* nulling speckles. Contains instances of ImageGrabber and P3KCom, as well as 
* a list of Speckle objects.
**/
class SpeckleNuller
{
    private: 
        cv::Mat image; //Last MKID image obtained from PacketMaster
        cv::Mat badPixMask; //Bad pixel mask of image. Should be updated when ctrl region changes
        cv::Mat probeFlatmap; //Flatmap storing probe speckles for current iteration 
        cv::Mat nullingFlatmap; //Flatmap storing nulling speckles
        //ImageGrabber imgGrabber; //ImageGrabber instance to interface with PacketMaster
        std::vector<Speckle> specklesList, nullSpecklesList; //List of Speckle (objects) being nulled
        boost::property_tree::ptree cfgParams; //Container for configuration params
        P3KCom *p3k; //P3KCom instance for loading flatmaps/centoffs into P3K
        bool verbose; //verbosity parameter; 1 if verbose, 0 if not
        //Speckle *speckleList;

    public:
        /**
        * Constructor. Initializes ImageGrabber and P3KCom objects.
        **/
        SpeckleNuller(boost::property_tree::ptree &ptree, bool vbose=false);

        /**
        * Detects and centroids speckles in current image.
        * @return vector of ImgPt objects containing speckle coordinates and intensities
        **/
        std::vector<ImgPt> detectSpeckles();

        /**
        * Cuts out elements of maxImgPts that are within an exclusion zone 
        * of existing speckles or each-other.
        **/
        void exclusionZoneCut(std::vector<ImgPt> &maxImgPts);

        /**
        * Checks whether any nulled speckles have been re-detected. If so,
        * updates speckle state and puts back in speckles list. Removes
        * point from maxImgPts.
        **/
        void updateAndCutNulledSpeckles(std::vector<ImgPt> &maxImgPts);

        void updateAndCutActiveSpeckles(std::vector<ImgPt> &maxImgPts);

        /**
        * Grabs a new image from packetmaster and updates current image
        * (stored in "image" array)
        * @param timestamp Start timestamp of image
        **/
        //void updateImage(uint64_t timestamp);
        
        /**
        * Updates the current image (of ctrl region)
        * @param image New image of ctrl region
        **/
        void updateImage(cv::Mat &newImage);

        void updateBadPixMask(cv::Mat &newMask);

        /**
        * Creates speckle objects from a list of ImgPts; stores these
        * in specklesList.
        **/
        void createSpeckleObjects(std::vector<ImgPt> &imgPts);

        void updateExistingSpeckles();

        /**
        * Measures the intensity of each speckle in specklesList at the specified probe phase; 
        * updates speckle objects with this measurement. For proper operation, probe speckles 
        * at the correct phase are assumed to have been applied in the current image.
        * @param phaseInd specifies the index of the probe phase in all speckle objects (index in Speckle.phaseList)
        **/
        void measureSpeckleProbeIntensities(int phaseInd);

        /**
        * Measures the intensity of each speckle in specklesList at the specified probe gain; 
        * updates speckle objects with this measurement. For proper operation, probe speckles 
        * at the correct gain are assumed to have been applied in the current image.
        * @param gainInd specifies the index of the probe gain in all speckle objects (index in Speckle.gainList)
        **/
        void measureSpeckleGainIntensities(int gainInd);

        /**
        * Run this right after final phase probe iteration. Updates speckle state and checks 
        * if SNR is high enough for nulling. If so, puts speckle in nullSpecklesList. Calculates
        * final phase and gain (if not using gain loop)
        **/
        void updateSpecklesAndCheckNull();

        /**
        * Calculates the nulling phase for all speckle objects. DEPRECATED - use updateSpecklesAndCheckNull()
        **/
        void calculateFinalPhases();

        /**
        * Determines the nulling gain for all speckle objects (if using gain loop)
        */
        void calculateFinalGains();

        /**
        * Generates probe speckle flatmap for each speckle at phase specified by phaseInd.
        * Adds them together and stores in probeFlatmap attribute.
        * @param phaseInd Index of speckle probe phase in Speckle.phaseList
        **/
        void generateProbeFlatmap(int phaseInd);

        /**
        * Overloaded for case when phaseInd is not the same for each speckle. (Code that
        * would use this is not yet implemented)
        * @param phaseInds List of indices of speckle probe phase in Speckle.phaseList
        **/
        void generateProbeFlatmap(std::vector<int> &phaseInds);

        /**
        * Generates probe speckle flatmap for each speckle at gain specified by gainInd.
        * Adds them together and stores in probeFlatmap attribute.
        * @param gainInd Index of speckle probe gain in Speckle.gainList
        **/
        void generateProbeGainFlatmap(int gainInd);

        /**
        * Generates nulling flatmap for each speckle in nullSpecklesList, sums them, 
        * and adds that to nullingFlatmap.
        */
        void generateNullingFlatmap();

        /**
        * Overloads generateNullingFlatmap if you want to specify a gain to use for all speckles.
        * @param gain Amplitude gain to use for all nulling speckles.
        **/
        void generateNullingFlatmap(double gain);

        /**
        * Grabs current flatmap from P3K to use as reference. NOT IMPLEMENTED
        **/
        void updateCurFlatmap();

        /**
        * Tools for interfacing with python simulation.
        **/
        void generateSimProbeSpeckles(int phaseInd);
        void generateSimFinalSpeckles();

        /**
        * Loads probe speckle flatmap into P3K.
        **/
        void loadProbeSpeckles();

        /**
        * Loads nulling speckle flatmap into P3K.
        **/
        void loadNullingSpeckles();

        /**
        * Removes all speckle objects from specklesList
        **/
        void clearSpeckleObjects();
        //void applyCorrection(

        /**
        * Initializes Kalman Filter 
	* JLlop
        **/
        void initKF();

        /**
        * Gets amplitude of the DM corresponding to a speckle
	* JLlop
        **/
        void calculateFinalAmplitude();

        /**
        * Updates estimates for Kalman Filter 
	* JLlop
        **/
        void updateEstimatesKF();

        /**
        * Updates control to DM with new estimates from Kalman Filter 
	* JLlop
        **/
        void updateControlKF();

};
#endif
