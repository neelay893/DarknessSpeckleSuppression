#include <opencv2/opencv.hpp>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include "params.h"
#include <boost/property_tree/ptree.hpp>

typedef double Pixel;

cv::Point2d calculateKVecs(const cv::Point2i &coords, boost::property_tree::ptree &cfgParams);
cv::Mat generateFlatmap(const cv::Point2d kvecs, unsigned short intensity, double phase, boost::property_tree::ptree &cfgParams);
cv::Mat generateFlatmap(const cv::Point2d kvecs, double amp, double phase);
double calculateDMAmplitude(const cv::Point2i &kvecs, unsigned short intensity);
cv::Mat convertFlatmapToCentoffs(const cv::Mat &flatmap, const cv::Mat &influenceMatrix);
cv::Mat clampCentoffs(cv::Mat &centoffs, double clamp);
cv::Mat applyIllumMatrix(cv::Mat &centoffs, cv::Mat &illumMat);
