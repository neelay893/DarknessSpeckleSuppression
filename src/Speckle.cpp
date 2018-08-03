#include "Speckle.h"

Speckle::Speckle(cv::Point2d &pt, boost::property_tree::ptree &ptree, bool verb)
{
    cfgParams = ptree;
    setCoordinates(pt);
    distribution = std::uniform_real_distribution<double>(0,1);
    for(int i=0; i<NPHASES; i++)
    {
        phaseList[i] = (double)2*M_PI*i/NPHASES;
        phaseIntensities[i] = 0;

    }
    if(cfgParams.get<bool>("NullingParams.useGainLoop"))
        for(int i=0; i<NGAINS; i++)
            gainList[i] = (double)(i+1)/NGAINS;

        
    apertureMask = cv::Mat::zeros(2*cfgParams.get<int>("NullingParams.apertureRadius")+1, 2*cfgParams.get<int>("NullingParams.apertureRadius")+1, CV_64F);
    cv::circle(apertureMask, cv::Point(cfgParams.get<int>("NullingParams.apertureRadius"), cfgParams.get<int>("NullingParams.apertureRadius")), cfgParams.get<int>("NullingParams.apertureRadius"), 1, -1);

    kvecOffsSize = cfgParams.get<double>("NullingParams.kvecOffsSize");
    rawKvecs = calculateKVecs(coordinates, cfgParams);
    kvecs = rawKvecs;
    if("NullingParams.useRandomKvecOffset")
        applyRandomKvecOffset();
    else
        measKvecs = rawKvecs;

    verbose = verb;
    isNulled = false;
    sigmaI = -1;
    nProbeIters = 0;

}


void Speckle::measurePhaseIntensity(int phaseInd, cv::Mat &image)
{
    measPhaseIntensities[phaseInd] = measureIntensity(image);

}

void Speckle::measureGainIntensity(int gainInd, cv::Mat &image)
{
    gainIntensities[gainInd] += measureIntensity(image);

}

void Speckle::measureSpeckleIntensityAndSigma(cv::Mat &image)
{
    measSpeckIntensity = measureIntensity(image);
    measSigmaI = std::sqrt(measSpeckIntensity*intensityCorrectionFactor)/intensityCorrectionFactor;
    if(sigmaI==-1)
        speckIntensity = measSpeckIntensity; //First iteration, so we don't have speckIntensity yet

}

double Speckle::measureIntensity(cv::Mat &image)
{
    cv::Mat speckleIm = cv::Mat(image, cv::Range((int)coordinates.y-cfgParams.get<int>("NullingParams.apertureRadius"), 
        (int)coordinates.y+cfgParams.get<int>("NullingParams.apertureRadius")+1), cv::Range(coordinates.x-cfgParams.get<int>("NullingParams.apertureRadius"), 
        coordinates.x+cfgParams.get<int>("NullingParams.apertureRadius")+1));
    speckleIm = speckleIm.mul(apertureMask);
    //std::cout << speckleIm << std::endl;
    return (double)cv::sum(speckleIm)[0]/intensityCorrectionFactor;

}

void Speckle::measureIntensityCorrection(cv::Mat &badPixMask)
{
    cv::Mat goodPixMask = (~badPixMask)&1;
    cv::Mat apertureGoodPixMask = cv::Mat(goodPixMask, cv::Range((int)coordinates.y-cfgParams.get<int>("NullingParams.apertureRadius"), 
        (int)coordinates.y+cfgParams.get<int>("NullingParams.apertureRadius")+1), cv::Range(coordinates.x-cfgParams.get<int>("NullingParams.apertureRadius"), 
        coordinates.x+cfgParams.get<int>("NullingParams.apertureRadius")+1));
    apertureGoodPixMask.convertTo(apertureGoodPixMask, CV_64F);
    cv::Mat gaussKernel = cv::getGaussianKernel(2*cfgParams.get<int>("NullingParams.apertureRadius")+1, cfgParams.get<double>("ImgParams.lambdaOverD")*0.42);
    gaussKernel = gaussKernel*gaussKernel.t();
    intensityCorrectionFactor = (double)cv::sum(gaussKernel.mul(apertureGoodPixMask))[0]/cv::sum(gaussKernel)[0];

}

void Speckle::updateStateEstimates()
{
    double w, wMeas;
    int i;

    measSpeckVisibility = 2*std::sqrt(std::pow(measPhaseIntensities[3]-measPhaseIntensities[1], 2) + std::pow(measPhaseIntensities[0]-measPhaseIntensities[2], 2))/(measPhaseIntensities[0]
        + measPhaseIntensities[1] + measPhaseIntensities[2] + measPhaseIntensities[3]);

    sigmaVis = cfgParams.get<double>("TrackingParams.sigmaVis0")*sigmaI/speckIntensity;
    measSigmaVis = cfgParams.get<double>("TrackingParams.sigmaVis0")*measSigmaI/measSpeckIntensity;
    
    if(sigmaI==-1)
        w = 0; //set sigmaI to have initial value of 1; don't use tracking parameters if first iteration
    else
        //w = (1/sigmaI)*(speckVisibility*(1-sigmaVis) + cfgParams.get<double>("TrackingParams.vis0")*sigmaVis); //weight of current state estimate
        w = std::pow(speckVisibility,2);

    if(isNulled)
    {   
        isNulled = false;
        w *= cfgParams.get<double>("TrackingParams.nullIterWeight");

    }

    if(measSpeckIntensity==0)
        wMeas = 0;

    else
        //wMeas = (cfgParams.get<double>("TrackingParams.iterGain")/measSigmaI)*(measSpeckVisibility*(1-measSigmaVis) //weight of prev measurement
        //    + cfgParams.get<double>("TrackingParams.vis0")*measSigmaVis);
        wMeas = std::pow(measSpeckVisibility, 2); //weight on just visibility for now

    speckIntensity = (w*speckIntensity + wMeas*measSpeckIntensity)/(w + wMeas);
    sigmaI = std::sqrt(std::pow(w,2)*std::pow(sigmaI,2) + std::pow(wMeas,2)*std::pow(measSigmaI,2))/(w + wMeas);
    

    for(i=0; i<NPHASES; i++)
        phaseIntensities[i] = (w*phaseIntensities[i] + wMeas*measPhaseIntensities[i])/(w + wMeas);

    // update speckle visibility using updated phase intensities
    speckVisibility = 2*std::sqrt(std::pow(phaseIntensities[3]-phaseIntensities[1], 2) + std::pow(phaseIntensities[0]-phaseIntensities[2], 2))/(phaseIntensities[0]
        + phaseIntensities[1] + phaseIntensities[2] + phaseIntensities[3]);

    nProbeIters++;

    if(cfgParams.get<bool>("NullingParams.useRandomKvecOffset"))
    {
        kvecs = (wMeas*measKvecs + w*kvecs)/(wMeas + w);
        std::cout << "Old measKvecs: " << measKvecs.x << " " << measKvecs.y <<std::endl;
        //kvecOffsSize = std::pow((1-2*speckVisibility), 2)*cfgParams.get<double>("NullingParams.kvecOffsSize");
        applyRandomKvecOffset();

    }


    if(verbose)
    {
        std::cout << "Updating speckle at " << coordinates.x << ", " << coordinates.y << std::endl;
        std::cout << "w:     " << w << std::endl;
        std::cout << "wMeas: " << wMeas << std::endl;
        std::cout << "Intensity: " << speckIntensity << std::endl;
        std::cout << "rawKvecs: " << rawKvecs.x << " " << rawKvecs.y <<std::endl;
        std::cout << "kvecs: " << kvecs.x << " " << kvecs.y <<std::endl;
        std::cout << "measKvecs: " << measKvecs.x << " " << measKvecs.y <<std::endl;
        std::cout << "Sigma I: " << sigmaI << std::endl;
        std::cout << "Meas Sigma I: " << measSigmaI << std::endl;
        std::cout << "phase intensities: " << phaseIntensities[0] << " " << phaseIntensities[1] << " " << phaseIntensities[2] << " " << phaseIntensities[3] << std::endl;
        std::cout << "meas phase intensities: " << measPhaseIntensities[0] << " " << measPhaseIntensities[1] << " " << measPhaseIntensities[2] << " " << measPhaseIntensities[3] << std::endl;
        std::cout << "Measured Intensity: " << measSpeckIntensity << std::endl;
        std::cout << "Visibility: " << speckVisibility << std::endl;
        std::cout << "Meas Visibility: " << measSpeckVisibility << std::endl;
        std::cout << "SNR: " << speckIntensity/sigmaI << std::endl;
        std::cout << "nProbeIters " << nProbeIters << std::endl;
        std::cout << std::endl;
    
    }


    

}

bool Speckle::checkToNull()
{
    if(speckIntensity/sigmaI < cfgParams.get<double>("TrackingParams.snrThresh"))
        return false;
    else if(nProbeIters < cfgParams.get<int>("TrackingParams.minProbeIters"))
        return false;
    else
        return true;

}

void Speckle::updateNulledSpeckle()
{
    int i;
    isNulled = true;
    double intensityFactor = std::pow((1-finalGain), 2);
    speckIntensity *= intensityFactor;
    sigmaI *= intensityFactor;
    for(i=0; i<NPHASES; i++)
        phaseIntensities[i] *= intensityFactor;

    //if(cfgParams.get<bool>("NullingParams.useRandomKvecOffset"))
    //    applyRandomKvecOffset();

}


void Speckle::calculateFinalPhase()
{
    //warning: current implementation only works for 4 phase measurements (0, pi/2, pi, 3pi/2)
    finalPhase = std::atan2(((double)phaseIntensities[1]-(double)phaseIntensities[3]), ((double)phaseIntensities[0]-(double)phaseIntensities[2]));
    std::cout << "raw final phase: " << finalPhase << std::endl;
   
}

void Speckle::calculateFinalGain()
{
    if(cfgParams.get<bool>("NullingParams.useGainLoop"))
    {
        double minIntensity = speckIntensity;
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

    else
    {
        finalGain = cfgParams.get<double>("NullingParams.defaultGain");
        if(cfgParams.get<bool>("NullingParams.scaleByVisibility"))
            finalGain *= speckVisibility;
        if(finalGain>1)
            finalGain = 1;
        //else if(speckVisibility < 0.2)
        //    finalGain = 0;

    }

}


cv::Mat Speckle::getProbeSpeckleFlatmap(int phaseInd)
{
    return generateFlatmap(measKvecs, speckIntensity, phaseList[phaseInd], cfgParams);

}

cv::Mat Speckle::getProbeGainSpeckleFlatmap(int gainInd)
{
    return generateFlatmap(kvecs, (unsigned short)(gainList[gainInd]*speckIntensity), finalPhase + M_PI, cfgParams);

}

cv::Mat Speckle::getFinalSpeckleFlatmap()
{
    return generateFlatmap(kvecs, (unsigned short)(finalGain*speckIntensity), finalPhase + M_PI, cfgParams);

}

void Speckle::generateSimProbeSpeckle(int phaseInd)
{
    writeToKvecFile(measKvecs, (double)std::sqrt(speckIntensity/cfgParams.get<double>("ImgParams.integrationTime"))*474, phaseList[phaseInd], 0);

}

void Speckle::generateSimFinalSpeckle()
{
    writeToKvecFile(kvecs, (double)(finalGain*std::sqrt(speckIntensity/cfgParams.get<double>("ImgParams.integrationTime"))*474), finalPhase + M_PI, 1);

}

void Speckle::applyRandomKvecOffset()
{
    kvecsOffs.x = kvecOffsSize*(2*M_PI/cfgParams.get<double>("ImgParams.lambdaOverD"))*(distribution(generator)-0.5); //currently offsets by up to 1.5 pix in x and y
    kvecsOffs.y = kvecOffsSize*(2*M_PI/cfgParams.get<double>("ImgParams.lambdaOverD"))*(distribution(generator)-0.5);
    measKvecs = rawKvecs + kvecsOffs;

}    

double Speckle::getFinalPhase() {return finalPhase;}

double Speckle::getFinalGain() {return finalGain;}

cv::Point2d Speckle::getCoordinates() {return coordinates;}

void Speckle::setCoordinates(cv::Point2d coords) 
{
    coordinates = coords;
    rawKvecs = calculateKVecs(coordinates, cfgParams);
    if(cfgParams.get<bool>("NullingParams.useRandomKvecOffset"))
        applyRandomKvecOffset();
    else
        kvecs = rawKvecs;
    
}

double Speckle::getIntensity() {return speckIntensity;}
