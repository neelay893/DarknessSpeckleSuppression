#include <opencv2/opencv.hpp>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include "params.h"

typedef float Pixel;

cv::Point2d calculateKVecs(const cv::Point2i &coords);
cv::Mat generateFlatmap(const cv::Point2d &kvecs, unsigned short intensity, double phase);
double calculateDMAmplitude(const cv::Point2i &kvecs, unsigned short intensity);
