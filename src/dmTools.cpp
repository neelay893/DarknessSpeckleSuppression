#include "dmTools.h"

cv::Point2d calculateKVecs(const cv::Point2i &coords, boost::property_tree::ptree &cfgParams)
{
    cv::Point2d intCoords, kvecs;
    double dmAngle = cfgParams.get<double>("DMCal.angle");
    intCoords.x = (double)(coords.x + cfgParams.get<int>("ImgParams.xCtrlStart"));
    intCoords.y = (double)(coords.y + cfgParams.get<int>("ImgParams.yCtrlStart"));
    kvecs.x = 2.0*M_PI*(std::cos(-dmAngle)*intCoords.x - std::sin(-dmAngle)*intCoords.y)/cfgParams.get<double>("ImgParams.lambdaOverD");
    kvecs.y = -2.0*M_PI*(std::sin(-dmAngle)*intCoords.x + std::cos(-dmAngle)*intCoords.y)/cfgParams.get<double>("ImgParams.lambdaOverD");
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
    std::cout << "dmTools: generating flatmap with amplitude: " << amp << std::endl;
    std::cout << "dmTools kvecs: " << kvecs << std::endl;
    std::cout << "dmTools phase: " << phase << std::endl;
    double phx, phy;
    cv::Mat flatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);
    /*flatmap.forEach<Pixel>([&phx, &phy, amp, phase, &kvecs](Pixel &value, const int *position) -> void
        { phy = (double)(((1.0/DM_SIZE)*position[0]-0.5)*kvecs.y);
          phx = (double)(((1.0/DM_SIZE)*position[1]-0.5)*kvecs.x);
          value = amp*std::cos(phx + phy + phase);

        });
    */

    return generateFlatmap(kvecs, amp, phase);

}

cv::Mat generateFlatmap(const cv::Point2d kvecs, double amp, double phase)
{
    double phx, phy;
    cv::Mat flatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);
    flatmap.forEach<Pixel>([&phx, &phy, amp, phase, &kvecs](Pixel &value, const int *position) -> void
        { phy = (double)(((1.0/DM_SIZE)*position[0]-0.5)*kvecs.y);
          phx = (double)(((1.0/DM_SIZE)*position[1]-0.5)*kvecs.x);
          value = amp*std::cos(phx + phy + phase);

        });
    
    
    return flatmap;

}

cv::Mat convertFlatmapToCentoffs(const cv::Mat &flatFlatmap, const cv::Mat &influenceMatrix)
{
    cv::Mat centoffs = influenceMatrix * flatFlatmap;
    return centoffs;

}

cv::Mat clampCentoffs(cv::Mat &centoffs, double clamp)
{
    centoffs.forEach<Pixel>([clamp](Pixel &value, const int *position)-> void
        { if(value>clamp) value=clamp;
          else if(value<-1*clamp) value=-1*clamp;

        });
        
    return centoffs;    
    
}

cv::Mat applyIllumMatrix(cv::Mat &centoffs, cv::Mat &illumMat)
{
    centoffs.forEach<Pixel>([&illumMat](Pixel &value, const int *position)-> void
        { value*=illumMat.at<double>(position[0]/2,0);});
        
    return centoffs;
        
}
