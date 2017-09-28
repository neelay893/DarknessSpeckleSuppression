#include "dmTools.h"

cv::Point2d calculateKVecs(const cv::Point2i &coords, boost::property_tree::ptree &cfgParams)
{
    cv::Point2d intCoords, kvecs;
    double dmAngle = cfgParams.get<double>("DMCal.angle");
    intCoords.x = (double)(coords.x + cfgParams.get<int>("ImgParams.xCtrlStart"));
    intCoords.y = (double)(coords.y + cfgParams.get<int>("ImgParams.yCtrlStart"));
    kvecs.x = 2.0*M_PI*(std::cos(-dmAngle)*intCoords.x - std::sin(-dmAngle)*intCoords.y)/cfgParams.get<double>("ImgParams.lambdaOverD");
    kvecs.y = 2.0*M_PI*(std::sin(-dmAngle)*intCoords.x + std::cos(-dmAngle)*intCoords.y)/cfgParams.get<double>("ImgParams.lambdaOverD");
    return kvecs;

}

double calculateDMAmplitude(const cv::Point2d &kvecs, unsigned short intensity, boost::property_tree::ptree &cfgParams)
{
    double k = norm(kvecs);
    return std::sqrt(intensity*(cfgParams.get<double>("DMCal.a")*k*k + cfgParams.get<double>("DMCal.b")*k + cfgParams.get<double>("DMCal.c")));

}


cv::Mat generateFlatmap(const cv::Point2d kvecs, unsigned short intensity, double phase, boost::property_tree::ptree &cfgParams)
{
    double amp = calculateDMAmplitude(kvecs, intensity, cfgParams);
    double phx, phy;
    cv::Mat flatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);
    flatmap.forEach<Pixel>([&phx, &phy, amp, phase, &kvecs](Pixel &value, const int *position) -> void
        { phy = (double)(((1.0/DM_SIZE)*position[0]-0.5)*kvecs.y*2*M_PI);
          phx = (double)(((1.0/DM_SIZE)*position[1]-0.5)*kvecs.x*2*M_PI);
          value = amp*std::cos(phx + phy + phase);

        });


    return flatmap;

}

cv::Mat generateFlatmap(const cv::Point2d kvecs, double amp, double phase)
{
    double phx, phy;
    cv::Mat flatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);
    flatmap.forEach<Pixel>([&phx, &phy, amp, phase, &kvecs](Pixel &value, const int *position) -> void
        { phy = (double)(((1.0/DM_SIZE)*position[0]-0.5)*kvecs.y*2*M_PI);
          phx = (double)(((1.0/DM_SIZE)*position[1]-0.5)*kvecs.x*2*M_PI);
          value = amp*std::cos(phx + phy + phase);

        });
    
    
    return flatmap;

}

cv::Mat convertFlatmapToCentoffs(const cv::Mat &flatmap, const cv::Mat &influenceMatrix)
{
    cv::Mat flatMapFlat = flatmap.reshape(1, DM_SIZE*DM_SIZE);
    cv::Mat centoffs = influenceMatrix * flatMapFlat;
    return centoffs;

}

cv::Mat clampCentoffs(cv::Mat &centoffs)
{
    return centoffs;    
    
}
