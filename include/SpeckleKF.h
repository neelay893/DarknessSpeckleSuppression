#include <opencv2/opencv.hpp>
#include <iostream>
#include <params.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <random>
#include <boost/property_tree/ptree.hpp>
#include "dmTools.h"
#include "simInterfaceTools.h"
#include "imageTools.h"

#ifndef SPECKLE_H
#define SPECKLE_H

/**
 * Each speckle object contains a single identified speckle. Stores
 * attributes such as intensity, position, and phase. Contains methods
 * for measuring the intensity, calculating gains/phases, and making 
 * DM maps.
 **/
class Speckle
{
    private:
        double phaseList[NPHASES]; //List of probe phases

        double phaseIntensities[NPHASES]; //measured speckle intensities at each probe phase
        double measPhaseIntensities[NPHASES]; //measured speckle intensities at each probe phase
        double speckIntensity; //measured intensity of original speckle
        double measSpeckIntensity; //measured intensity of original speckle
        double intensityCorrectionFactor;
        double sigmaI;
        double measSigmaI;
        double sigmaVis;
        double measSigmaVis;
        double speckVisibility;
        double measSpeckVisibility;
        cv::Point2d coordinates;
        cv::Point2d measCoordinates;
        
        bool isNulled; //True if speckle was nulled in last iteration

        double finalPhase; //phase of "nulling" speckle
        double finalGain; //gain of "nulling" speckle
        double gainIntensities[NGAINS]; //measured speckle intensities at each probe gain
        double gainList[NGAINS]; //List of probe gains, if using gain loop
        cv::Point2d kvecs; //speckle k-vectors (spatial angular frequencies)
        cv::Point2d rawKvecs; //speckle k-vectors (spatial angular frequencies)
        cv::Point2d kvecsOffs; //speckle k-vectors (spatial angular frequencies)
        cv::Mat apertureMask; //Aperture window used to measure speckle intensity
        boost::property_tree::ptree cfgParams; //container used to store configuration parameters
        void computeSpecklePhase(); //method to compute nulling phase
        double measureIntensity(cv::Mat &image); 

        std::default_random_engine generator;
        std::uniform_real_distribution<double> distribution;

        bool verbose;
	
	//JLlop: Kalman Filter parameters:
	double finalAmplitude;
	cv::Mat processNoiseMatKF;
	cv::Mat measurementNoiseMatKF;
	cv::Mat observationMatKF;
	cv::Mat interactionMatKF;
	cv::Mat covarianceMatPriorKF;
	cv::Mat covarianceMatCurrentKF;
	cv::Mat estimatePreviousKF;
	cv::Mat estimateCurrentKF;
	cv::Mat controlKF;

    public:
        /**
        * Constructor. Calculates kvecs from provided array coordinates, and PSF coordinates 
        * provided in config params. 
        * @param pt Speckle coordinates on the array
        * @param ptree Property tree of config parameters
        **/
        Speckle(cv::Point2d &pt, boost::property_tree::ptree &ptree, bool verb=true);

        /**
        * Measures the speckle intensity in the provided image, then sets 
        * measPhaseIntensities[phaseInd] to this value.
        * @param phaseInd Index of probe phase.
        * @param image Image of ctrl region to use for intensity measurement
        **/
        void measurePhaseIntensity(int phaseInd, cv::Mat &image);

        /**
        * Measures the speckle intensity in the provided image, then sets 
        * gainIntensities[gainInd] to this value.
        * @param gainInd Index of probe gain.
        * @param image Image of ctrl region to use for intensity measurement
        **/
        void measureGainIntensity(int gainInd, cv::Mat &image);

        /**
        * Measures the speckle intensity in the provided image, then calculates
        * sigma and sets measSpeckIntensity and measSigma
        * @param image Image to use for intensity measurement
        **/
        void measureSpeckleIntensityAndSigma(cv::Mat &image);

        void remeasureCoordinates(cv::Mat &image);

        void measureIntensityCorrection(cv::Mat &badPixMask);

        void updateStateEstimates();

        void applyRandomKvecOffset();

        bool checkToNull();

        void updateNulledSpeckle();

        /**
        * Calculate the (nulling) phase of the speckle from all of
        * the probe measurements. Stores result in finalPhase attribute.
        **/
        void calculateFinalPhase();

        /**
        * Calculate the gain of the applied nulling speckle. Only used
        * in null gain loop. Currently just picks the gain resulting in the
        * minimum total speckle intensity; does no fitting. Stores result
        * in finalGain attribute.
        **/
        void calculateFinalGain();

        /**
        * Returns flatmap for a probe speckle (at the original speckle's location)
        * with phase specified by phaseInd. All speckle amplitudes calculated using
        * calibration parameters specified in cfgParams.
        * @param phaseInd Index of probe phase in phaseList.
        **/
        cv::Mat getProbeSpeckleFlatmap(int phaseInd);

        /**
        * Returns flatmap for a probe speckle at the nulling phase, with amplitude
        * gain specified by gainInd
        * @param gainInd Index of probe gain in gainList
        **/
        cv::Mat getProbeGainSpeckleFlatmap(int gainInd);

        /**
        * Returns flatmap for the final "nulling" speckle at the nulling phase and
        * specified gain.
        * @param gain Amplitude gain of applied speckle
        **/
        cv::Mat getFinalSpeckleFlatmap();

        /**
        * Tools for interfacing with python simulation
        **/
        void generateSimProbeSpeckle(int phaseInd);
        void generateSimFinalSpeckle();

        void setCoordinates(cv::Point2d coords);

        /**
        * Return speckle location in pixel coordinates on the array.
        **/
        cv::Point2d getCoordinates();

        /* returns finalGain */
        double getFinalGain();

        /* returns finalPhase */
        double getFinalPhase();

        double getIntensity();

	/* returns amplitude corresponding to Speckle. JLlop */
	double getFinalAmplitude();

	/* sets amplitude. For Kalman Filter. JLlop */
	void setFinalAmplitude(double amp);

	/* sets phase. For Kalman Filter. JLlop */
	void setFinalPhase(double phase);

	/* returns process noise matrix. JLlop */
	cv::Mat getProcessNoiseMatKF();

	/* returns measurement noise matrix. JLlop */
	cv::Mat getMeasurementNoiseMatKF();

	/* returns observation matrix. JLlop */
	cv::Mat getObservationMatKF();

	/* returns interaction matrix. JLlop */
	cv::Mat getInteractionMatKF();

	/* returns prior covariance matrix. JLlop */
	cv::Mat getCovarianceMatPriorKF();

	/* returns current covariance matrix. JLlop */
	cv::Mat getCovarianceMatCurrentKF();

	/* returns previous estimate. JLlop */
	cv::Mat getEstimatePreviousKF();

	/* returns current estimate. JLlop */
	cv::Mat getEstimateCurrentKF();

	/* returns control from KF. JLlop */
	cv::Mat getControlKF();

	/* sets process noise matrix. JLlop */
	void setProcessNoiseMatKF(cv::Mat mat);

	/* set measurement noise matrix. JLlop */
	void setMeasurementNoiseMatKF(cv::Mat mat);

	/* set observation matrix. JLlop */
	void setObservationMatKF(cv::Mat mat);

	/* set interaction matrix. JLlop */
	void setInteractionMatKF(cv::Mat mat);

	/* set prior covariance matrix. JLlop */
	void setCovarianceMatPriorKF(cv::Mat mat);

	/* set current covariance matrix. JLlop */
	void setCovarianceMatCurrentKF(cv::Mat mat);

	/* set previous estimate. JLlop */
	void setEstimatePreviousKF(cv::Mat mat);

	/* set current estimate. JLlop */
	void setEstimateCurrentKF(cv::Mat mat);

	/* set control from KF. JLlop */
	void setControlKF(cv::Mat mat);


};
#endif
