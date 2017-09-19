#include "dmTools.h"

cv::Point2d calculateKVecs(const cv::Point2i &coords)
{
    cv::Point2d intCoords, kvecs;
    intCoords.x = (double)(coords.x + XCTRLSTART);
    intCoords.y = (double)(coords.y + YCTRLSTART);
    kvecs.x = 2.0*M_PI*(std::cos(-DM_ANGLE)*intCoords.x - std::sin(-DM_ANGLE)*intCoords.y)/(double)LAMBDAOVERD;
    kvecs.y = 2.0*M_PI*(std::sin(-DM_ANGLE)*intCoords.x + std::cos(-DM_ANGLE)*intCoords.y)/(double)LAMBDAOVERD;
    return kvecs;

}

double calculateDMAmplitude(const cv::Point2d &kvecs, unsigned short intensity)
{
    double k = norm(kvecs);
    return std::sqrt(intensity*(AMP_A*k*k + AMP_B*k + AMP_C));

}

cv::Mat generateFlatmap(const cv::Point2d &kvecs, unsigned short intensity, double phase)
{
    double amp = calculateDMAmplitude(kvecs, intensity);
    double phx, phy;
    cv::Mat flatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);
    flatmap.forEach<Pixel>([&phx, &phy, amp, phase, &kvecs](Pixel &value, const int *position) -> void
        { phy = (double)(((1.0/DM_SIZE)*position[0]-0.5)*kvecs.y*2*M_PI);
          phx = (double)(((1.0/DM_SIZE)*position[1]-0.5)*kvecs.x*2*M_PI);
          value = amp*std::cos(phx + phy + phase);

        });


    return flatmap;

}

cv::Mat convertFlatmapToCentOffs(const cv::Mat &flatmap, const cv::Mat &influenceMatrix)
{
    cv::Mat flatMapFlat = flatmap.reshape(1, DM_SIZE*DM_SIZE);
    cv::Mat centoffs = influenceMatrix * flatMapFlat;
    return centoffs;

}
