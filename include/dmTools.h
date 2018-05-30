/**
* dmTools: set of functions for generating/converting flatmaps, centoffs, etc.
**/

#include <opencv2/opencv.hpp>
#define _USE_MATH_DEFINES
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <opencv2/core/eigen.hpp>
#include <cmath>
#include <iostream>
#include "params.h"
#include <boost/property_tree/ptree.hpp>

typedef double Pixel;

/**
* Calculates speckle k-vectors (spatial frequencies) from coordinates. PSF location is provided by the 
* config params.
* @param coords Speckle coordinates on array
* @param cfgParams Configuration parameters. Relevant ones are lambda/D and PSF location on the array
**/
cv::Point2d calculateKVecs(const cv::Point2d &coords, boost::property_tree::ptree &cfgParams);

/**
* Generates a DM flatmap from speckle kvecs, intensity, and phase. Calibration is provided by config params
* @param kvecs Speckle k-vectors
* @param intensity Speckle intensity
* @param phase Speckle phase
* @param Configuration parameters. Relevant ones are intensity cal params (DMCal section)
* @returns 2D cv::Mat of actuator heights.
**/
cv::Mat generateFlatmap(const cv::Point2d kvecs, unsigned short intensity, double phase, boost::property_tree::ptree &cfgParams);

/**
* Overloads generateFlatmap for manual specification of DM amplitude.
* @param kvecs Speckle k-vectors
* @param amplitude DM amplitude
* @param phase Speckle phase
* @returns 2D cv::Mat of actuator heights.
**/
cv::Mat generateFlatmap(const cv::Point2d kvecs, double amp, double phase);

/**
* Calculates DM amplitude from calibration.
* @param kvecs Speckle k-vectors
* @param intensity Speckle intensity
* NOTE: ISN'T ACTUALLY IMPLEMENTED
**/
double calculateDMAmplitude(const cv::Point2i &kvecs, unsigned short intensity);

/**
* Converts flatmap to list of centroid offsets using supplied influence matrix. Basically
* just does a matrix multiplication. 
* @param flatmap DM flatmap
* @param P3K influence matrix
* @return cv::Mat list of centroid offsets
**/
cv::Mat convertFlatmapToCentoffs(const cv::Mat &flatmap, const cv::Mat &influenceMatrix);
cv::Mat convertFlatmapToCentoffsSparse(const cv::Mat &flatFlatmap, const Eigen::SparseMatrix<float> &influenceMatrix);

/**
* Clamps the centroid offsets; i.e. causes them to saturate at specified clamp value.
* @param centoffs list of centroid offsets
* @param clamp Clamp (saturation) value
**/
cv::Mat clampCentoffs(cv::Mat &centoffs, double clamp);

/**
* Applies the illumination matrix to centroid offset list
* @param centoffs List of centroid offsets
* @param illumMat Illumination matrix
**/
cv::Mat applyIllumMatrix(cv::Mat &centoffs, cv::Mat &illumMat);
