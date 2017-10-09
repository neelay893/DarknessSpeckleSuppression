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
    if(cfgParams.get<bool>("NullingParams.useGainLoop"))
        for(int i=0; i<NGAINS; i++)
            gainList[i] = (double)(i+1)/NGAINS;

        
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

void Speckle::incrementGainIntensity(int gainInd, unsigned int intensity)
{
    gainIntensities[gainInd] += intensity;

}

void Speckle::calculateFinalPhase()
{
    //warning: current implementation only works for 4 phase measurements (0, pi/2, pi, 3pi/2)
    finalPhase = std::atan2(((double)phaseIntensities[3]-(double)phaseIntensities[1]), ((double)phaseIntensities[0]-(double)phaseIntensities[2]));
    std::cout << "phase intensities: " << phaseIntensities[0] << " " << phaseIntensities[1] << " " << phaseIntensities[2] << " " << phaseIntensities[3] << std::endl;
    std::cout << "probe speckle intensity: " << ((int)phaseIntensities[3] + phaseIntensities[1])/2 - (int)initialIntensity << std::endl;
    std::cout << "probe speckle intensity (2): " << ((int)phaseIntensities[0] + phaseIntensities[2])/2 - (int)initialIntensity << std::endl;
    std::cout << "final phase " << finalPhase << std::endl;
   
}

void Speckle::calculateFinalGain()
{
    unsigned int minIntensity = initialIntensity;
    finalGain = 0; 
    for(int i=0; i<NGAINS; i++)
    {
        if(gainIntensities[i]<minIntensity)
        {
            minIntensity = gainIntensities[i];
            finalGain = gainList[i];

        }

    }

}


cv::Mat Speckle::getProbeSpeckleFlatmap(int phaseInd)
{
    std::cout << "Speckle probe phase: " << phaseList[phaseInd] << std::endl;
    return generateFlatmap(kvecs, initialIntensity, phaseList[phaseInd], cfgParams);

}

cv::Mat Speckle::getProbeGainSpeckleFlatmap(int gainInd)
{
    std::cout << "Speckle probe gain: " << gainList[gainInd] << std::endl;
    return generateFlatmap(kvecs, (unsigned short)(gainList[gainInd]*initialIntensity), finalPhase + M_PI, cfgParams);

}

cv::Mat Speckle::getFinalSpeckleFlatmap(double gain)
{
    std::cout << "Speckle null phase: " << finalPhase + M_PI << std::endl;
    double visibility = 2*std::sqrt(std::pow((double)phaseIntensities[3]-phaseIntensities[1], 2) + std::pow((double)phaseIntensities[0]-phaseIntensities[2], 2))/((double)phaseIntensities[0]
        + phaseIntensities[1] + phaseIntensities[2] + phaseIntensities[3]);
    std::cout << "Speckle visibility: " << visibility << std::endl;
    if(cfgParams.get<bool>("NullingParams.scaleByVisibility"))
        return generateFlatmap(kvecs, (unsigned short)(visibility*gain*initialIntensity), finalPhase + M_PI, cfgParams);
    else
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

double Speckle::getFinalGain() {return finalGain;}

cv::Point2i Speckle::getCoordinates() {return coordinates;}
