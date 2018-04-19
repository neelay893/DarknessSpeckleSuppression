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
        double gainList[NGAINS]; //List of probe gains, if using gain loop
        unsigned int phaseIntensities[NPHASES]; //measured speckle intensities at each probe phase
        unsigned int gainIntensities[NGAINS]; //measured speckle intensities at each probe gain
        unsigned int initialIntensity; //measured intensity of original speckle
        double finalPhase; //phase of "nulling" speckle
        double finalGain; //gain of "nulling" speckle
        double nullingIntensity; 
        cv::Point2i coordinates;
        cv::Point2d kvecs; //speckle k-vectors (spatial angular frequencies)
        cv::Mat apertureMask; //Aperture window used to measure speckle intensity
        boost::property_tree::ptree cfgParams; //container used to store configuration parameters
        void computeSpecklePhase(); //method to compute nulling phase

    public:
        /**
        * Constructor. Calculates kvecs from provided array coordinates, and PSF coordinates 
        * provided in config params. 
        * @param pt Speckle coordinates on the array
        * @param ptree Property tree of config parameters
        **/
        Speckle(cv::Point2i &pt, boost::property_tree::ptree &ptree);

        /**
        * Increments the measured speckle intensity at a specified probe phase by the specified
        * amount. Use this to set probe phase speckle intensites.
        * @param phaseInd Index of probe phase.
        * @param intensity Amount to increment probe phase intensity.
        **/
        void incrementPhaseIntensity(int phaseInd, unsigned int intensity);

        /**
        * Increments the measured speckle intensity at a specified probe gain by the specified
        * amount. Use this to set probe gain speckle intensites.
        * @param gainInd Index of probe gain.
        * @param intensity Amount to increment probe gain intensity.
        **/
        void incrementGainIntensity(int gainInd, unsigned int intensity);

        /**
        * Sets the intensity of the original speckle.
        * @param intensity Intensity of the speckle.
        **/
        void setInitialIntensity(unsigned int intensity);

        /**
        * Measures the intensity of the speckle.
        * @param image Image of control region.
        **/
        unsigned int measureSpeckleIntensity(cv::Mat &image);

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

        /* returns finalGain */
        double getFinalGain();

        /* returns finalPhase */
        double getFinalPhase();

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
        cv::Mat getFinalSpeckleFlatmap(double gain=DEFAULTGAIN);

        /**
        * Tools for interfacing with python simulation
        **/
        void generateSimProbeSpeckle(int phaseInd);
        void generateSimFinalSpeckle(double gain);

        /**
        * Return speckle location in pixel coordinates on the array.
        **/
        cv::Point2i getCoordinates();

};
#endif
