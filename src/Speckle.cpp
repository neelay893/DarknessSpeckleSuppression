#include "Speckle.h"

Speckle::Speckle(cv::Point2i &pt)
{
    coordinates = pt;
    for(int i=0; i<NPHASES; i++)
    {
        phaseList[i] = (double)2*M_PI*i/NPHASES;
        phaseIntensities[i] = 0;

    }
    apertureMask = cv::Mat::zeros(2*SPECKLEAPERTURERADIUS+1, 2*SPECKLEAPERTURERADIUS+1, CV_16UC1);
    cv::circle(apertureMask, cv::Point(SPECKLEAPERTURERADIUS, SPECKLEAPERTURERADIUS), SPECKLEAPERTURERADIUS, 1, -1);
    kvecs = calculateKVecs(coordinates);

}

unsigned short Speckle::measureSpeckleIntensity(cv::Mat &image)
{
    cv::Mat speckleIm = cv::Mat(image, cv::Range(coordinates.y-SPECKLEAPERTURERADIUS, coordinates.y+SPECKLEAPERTURERADIUS+1),
                            cv::Range(coordinates.x-SPECKLEAPERTURERADIUS, coordinates.x+SPECKLEAPERTURERADIUS+1));
    speckleIm = speckleIm.mul(apertureMask);
    //std::cout << speckleIm << std::endl;
    return (unsigned short)cv::sum(speckleIm)[0];

}

void Speckle::incrementPhaseIntensity(int phaseInd, unsigned short intensity)
{
    phaseIntensities[phaseInd] += intensity;

}

void Speckle::calculateFinalPhase()
{
    //warning: current implementation only works for 4 phase measurements (0, pi/2, pi, 3pi/2)
    finalPhase = std::atan2((double)(phaseIntensities[3]-phaseIntensities[1]), (double)(phaseIntensities[0]-phaseIntensities[2]));
   
}

cv::Mat Speckle::getProbeSpeckleFlatmap(int phaseInd)
{
    return generateFlatmap(kvecs, initialIntensity, phaseList[phaseInd]);

}

cv::Mat Speckle::getFinalSpeckleFlatmap(double gain)
{
    return generateFlatmap(kvecs, (unsigned short)(gain*initialIntensity), finalPhase + M_PI);

}

void Speckle::generateSimProbeSpeckle(int phaseInd)
{
    writeToKvecFile(kvecs, (double)(initialIntensity)/300, phaseList[phaseInd], 0);

}

void Speckle::generateSimFinalSpeckle(double gain)
{
    writeToKvecFile(kvecs, (double)(gain*initialIntensity/300), -1*finalPhase + M_PI, 1);

}

void Speckle::setInitialIntensity(unsigned short intensity) {initialIntensity = intensity;}

double Speckle::getFinalPhase() {return finalPhase;}

cv::Point2i Speckle::getCoordinates() {return coordinates;}
