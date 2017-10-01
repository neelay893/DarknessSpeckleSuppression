#include "Speckle.h"

Speckle::Speckle(cv::Point2i &pt, boost::property_tree::ptree &ptree)
{
    cfgParams = ptree;
    coordinates = pt;
    for(int i=0; i<NPHASES; i++)
    {
        phaseList[i] = (double)2*M_PI*i/NPHASES;
        phaseIntensities[i] = 0;

    }
    apertureMask = cv::Mat::zeros(2*cfgParams.get<int>("NullingParams.apertureRadius")+1, 2*cfgParams.get<int>("NullingParams.apertureRadius")+1, CV_16UC1);
    cv::circle(apertureMask, cv::Point(cfgParams.get<int>("NullingParams.apertureRadius"), cfgParams.get<int>("NullingParams.apertureRadius")), cfgParams.get<int>("NullingParams.apertureRadius"), 1, -1);
    kvecs = calculateKVecs(coordinates, cfgParams);

}

unsigned int Speckle::measureSpeckleIntensity(cv::Mat &image)
{
    cv::Mat speckleIm = cv::Mat(image, cv::Range(coordinates.y-cfgParams.get<int>("NullingParams.apertureRadius"), coordinates.y+cfgParams.get<int>("NullingParams.apertureRadius")+1),
                            cv::Range(coordinates.x-cfgParams.get<int>("NullingParams.apertureRadius"), coordinates.x+cfgParams.get<int>("NullingParams.apertureRadius")+1));
    speckleIm = speckleIm.mul(apertureMask);
    speckleIm.convertTo(speckleIm, CV_32SC1);
    return (unsigned int)cv::sum(speckleIm)[0];

}

void Speckle::incrementPhaseIntensity(int phaseInd, unsigned int intensity)
{
    phaseIntensities[phaseInd] += intensity;

}

void Speckle::calculateFinalPhase()
{
    //warning: current implementation only works for 4 phase measurements (0, pi/2, pi, 3pi/2)
    finalPhase = std::atan2(((double)phaseIntensities[3]-(double)phaseIntensities[1]), ((double)phaseIntensities[0]-(double)phaseIntensities[2]));
    std::cout << "phase intensities: " << phaseIntensities[0] << " " << phaseIntensities[1] << " " << phaseIntensities[2] << " " << phaseIntensities[3] << std::endl;
    std::cout << "final phase " << finalPhase << std::endl;
   
}

cv::Mat Speckle::getProbeSpeckleFlatmap(int phaseInd)
{
    std::cout << "Speckle probe phase: " << phaseList[phaseInd] << std::endl;
    return generateFlatmap(kvecs, initialIntensity, phaseList[phaseInd], cfgParams);

}

cv::Mat Speckle::getFinalSpeckleFlatmap(double gain)
{
    std::cout << "Speckle null phase: " << finalPhase + M_PI << std::endl;
    return generateFlatmap(kvecs, (unsigned short)(gain*initialIntensity), finalPhase + M_PI, cfgParams);

}

void Speckle::generateSimProbeSpeckle(int phaseInd)
{
    writeToKvecFile(kvecs, (double)(initialIntensity)/300, phaseList[phaseInd], 0);

}

void Speckle::generateSimFinalSpeckle(double gain)
{
    writeToKvecFile(kvecs, (double)(gain*initialIntensity/300), -1*finalPhase + M_PI, 1);

}

void Speckle::setInitialIntensity(unsigned int intensity) {initialIntensity = intensity;}

double Speckle::getFinalPhase() {return finalPhase;}

cv::Point2i Speckle::getCoordinates() {return coordinates;}
