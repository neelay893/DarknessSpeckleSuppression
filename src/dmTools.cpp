#include "dmTools.h"

cv::Point2i calculateKVecs(const cv::Point2i &coords)
{
    cv::Point2d intCoords, kvecs;
    intCoords.x = (double)(coords.x + XCTRLSTART);
    intCoords.y = (double)(coords.y + YCTRLSTART);
    kvecs.x = (std::cos(-ANGLE)*intCoords.x - std::sin(-ANGLE)*intCoords.y)/(double)LAMBDAOVERD;
    kvecs.y = (std::sin(-ANGLE)*intCoords.x + std::cos(-ANGLE)*intCoords.y)/(double)LAMBDAOVERD;
    return kvecs;

}

double calculateDMAmplitude(const cv::Point2d &kvecs, unsigned short intensity)
{
    double k = norm(kvecs);
    return std::sqrt(intensity*(A*k*k + B*k + C));

}

cv::Mat generateFlatmap(const cv::Point2d &kvecs, unsigned short intensity, double phase)
{
    double amp = calculateDMAmplitude(kvecs, intensity);
    double phx, phy;
    cv::Mat flatmap = cv::Mat::zeros(SIZE, SIZE, CV_64F);
    flatmap.forEach<Pixel>([&phx, &phy, amp, phase, &kvecs](Pixel &value, const int *position) -> void
        { phy = (double)(((1.0/SIZE)*position[0]-0.5)*kvecs.y*2*M_PI);
          phx = (double)(((1.0/SIZE)*position[1]-0.5)*kvecs.x*2*M_PI);
          value = amp*std::cos(phx + phy + phase);

        });

    return flatmap;

}

cv::Mat convertFlatmapToCentOffs(const cv::Mat &flatmap, const cv::Mat &influenceMatrix)
{
    cv::Mat flatMapFlat = flatmap.reshape(1, SIZE*SIZE);
    cv::Mat centoffs = influenceMatrix * flatMapFlat;
    return centoffs;

}
