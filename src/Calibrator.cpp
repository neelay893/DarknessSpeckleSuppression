#include "Calibrator.h"

Calibrator::Calibrator(boost::property_tree::ptree &ptree, P3KCom *p)
{
    p3k = p;
    cfgParams = ptree;
    nullingFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);
    lambdaOverD = cfgParams.get<double>("ImgParams.lambdaOverD");
    dmAngle = cfgParams.get<double>("DMCal.angle");
    imgCenter = cv::Point2d(cfgParams.get<double>("ImgParams.xCenter"), cfgParams.get<double>("ImgParams.yCenter"));
    speckleFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);
    verbose = true;
    trackingIter = 0;
    imgCenterVar = cv::Mat::ones(2,1);


}

void Calibrator::updateImage(cv::Mat img, bool addToCurImg)
{
    if(addToCurImg)
        image += img;
    else
        image = img;
    filterImage(image);

}

void Calibrator::updateBadPixMask(cv::Mat &bpMask)
{
    badPixMask = bpMask;

}

void Calibrator::filterImage(cv::Mat &img)
{
    filtImage = gaussianBadPixUSFilt(image, badPixMask, cfgParams.get<int>("ImgParams.usFactor"), cfgParams.get<double>("ImgParams.lambdaOverD"));

}


void Calibrator::updateNullingFlatmap(cv::Mat &flatmap)
{
    nullingFlatmap = flatmap;

}

void Calibrator::determineImgCenter()
{
    cv::Point2d imCent(0,0);
    int nKvecsUsed = 0; //number of kvecs used to determine center
    for(int i=0; i<curKvecs.size(); i++)
    {
        if(isValidPos[i] && isValidNeg[i])
        {
            imCent += (curTruePos[i] + curTrueNeg[i])/2;
            nKvecsUsed++;

        }

    }

    if(nKvecsUsed>0)
    {
        imCent = imCent/(double)nKvecsUsed;
        imgCenter = imCent;

    }

    else
        imCent = cv::Point2d(-1, -1); //return this if no valid center was calculated


}

void Calibrator::determineLambdaOverD()
{
    double curLOverD;
    int nKvecsUsed = 0; //number of kvecs used to determine angle
    cv::Point2d posDiffVec;
    for(int i=0; i<curKvecs.size(); i++)
    {
        if(isValidPos[i] && isValidNeg[i])
        {
            posDiffVec = curTruePos[i] - curTrueNeg[i];
            curLOverD += cv::norm(posDiffVec)*2*M_PI/cv::norm(2*curKvecs[i]);
            nKvecsUsed++;

        }

    }

    if(nKvecsUsed>0)
    {
        curLOverD /= (double)nKvecsUsed;
        lambdaOverD = curLOverD;

    }


}

void Calibrator::determineDMAngle()
{
    double angle = 0;
    double curAngle;
    int nKvecsUsed = 0; //number of kvecs used to determine angle
    cv::Point2d posDiffVec;
    double posDiffAngle;
    double kvecAngle;
    for(int i=0; i<curKvecs.size(); i++)
    {
        if(isValidPos[i] && isValidNeg[i])
        {
            posDiffVec = curTruePos[i] - curTrueNeg[i];
            posDiffAngle = std::atan2(posDiffVec.y, posDiffVec.x);
            kvecAngle = std::atan2(curKvecs[i].y, curKvecs[i].x);
            std::cout << "posDiffAngle: " << posDiffAngle << std::endl;
            std::cout << "kvecAngle: " << kvecAngle << std::endl;
            angle += (posDiffAngle + kvecAngle);
            nKvecsUsed++;

        }

    }

    if(nKvecsUsed>0)
    {
        angle /= (double)nKvecsUsed;
        dmAngle = angle;

    }

    else
        angle = 10;


}

void Calibrator::centroidCalSpeckles()
{ 
    for(int i=0; i<curKvecs.size(); i++)
    {
        if(isValidPos[i])
            curTruePos[i] = centroidPoint(curPosEstimates[i]);
        if(isValidNeg[i])
            curTrueNeg[i] = centroidPoint(curNegEstimates[i]);

        if(verbose)
        {
            std::cout << "Calibrator: centroiding speckle: " << curKvecs[i].x << " " << curKvecs[i].y << std::endl;
            std::cout << "  positive speckle at: " << curTruePos[i].x << " " << curTruePos[i].y << std::endl;
            std::cout << "  negative speckle at: " << curTrueNeg[i].x << " " << curTrueNeg[i].y << std::endl;

        }

    }

}
    


cv::Point2d Calibrator::centroidPoint(cv::Point2d posEstimate)
{
    int centroidingWindow = cfgParams.get<int>("ImgParams.usFactor")*cfgParams.get<int>("CalParams.centroidingWindowRadius");
    posEstimate = 2*posEstimate;
    std::cout << "posEstimate: " << posEstimate/2 << std::endl;
    cv::Mat speckleIm = cv::Mat(filtImage, cv::Range((int)posEstimate.y-centroidingWindow, (int)posEstimate.y+centroidingWindow+1),
        cv::Range((int)posEstimate.x-centroidingWindow, (int)posEstimate.x+centroidingWindow+1));

    cv::Point2i minloc, maxloc, maxCoords;
    double min, max;
    cv::minMaxLoc(speckleIm, &min, &max, &minloc, &maxloc);
    maxCoords = posEstimate - cv::Point2d((double)centroidingWindow, (double)centroidingWindow) 
        + cv::Point2d((double)maxloc.x, (double)maxloc.y);
    return maxCoords/cfgParams.get<double>("ImgParams.usFactor");

}

cv::Point2d Calibrator::calculatePositionEstimate(cv::Point2d kvec)
{
    cv::Point2d coords;
    kvec = kvec*lambdaOverD/(2*M_PI); //scale kvec
    kvec.y = -1*kvec.y; //flip when on P3K (I think....)
    coords.x = std::cos(dmAngle)*kvec.x - std::sin(dmAngle)*kvec.y;
    coords.y = std::sin(dmAngle)*kvec.x + std::cos(dmAngle)*kvec.y;
    return coords;

}


void Calibrator::addCalSpeckle(cv::Point2d kvec, double amplitude)
{
    curKvecs.push_back(kvec);
    dmAmplitudes.push_back(amplitude);
    cv::Point2d posEst = imgCenter + calculatePositionEstimate(kvec);
    cv::Point2d negEst = imgCenter - calculatePositionEstimate(kvec);  
    curPosEstimates.push_back(posEst);
    curNegEstimates.push_back(negEst);
    curTruePos.push_back(cv::Point2d(-1,-1));
    curTrueNeg.push_back(cv::Point2d(-1,-1));
    posSpeckleIntensities.push_back(-1);
    negSpeckleIntensities.push_back(-1);

    if(verbose)
    {
        std::cout << "Calibrtor: creating speckle with kvec: " << kvec.x << " " << kvec.y << std::endl;
        std::cout << "  Estimated postion: " << posEst.x << " " << posEst.y << std::endl << std::endl;

    }

    if((posEst.x >= 0) && (posEst.x < IMXSIZE) && (posEst.y >= 0) && (posEst.y < IMYSIZE))
        isValidPos.push_back(true);
    else
        isValidPos.push_back(false);

    if((negEst.x >= 0) && (negEst.x < IMXSIZE) && (negEst.y >= 0) && (negEst.y < IMYSIZE))
        isValidNeg.push_back(true);
    else
        isValidNeg.push_back(false);

    speckleFlatmap += generateFlatmap(kvec, amplitude, 0);

}
    
void Calibrator::clearCalSpeckles()
{
    curKvecs.clear();
    dmAmplitudes.clear();
    curPosEstimates.clear();
    curNegEstimates.clear();
    isValidPos.clear();
    isValidNeg.clear();
    curTruePos.clear();
    curTrueNeg.clear();
    posSpeckleIntensities.clear();
    negSpeckleIntensities.clear();
    speckleFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);
    loadCalFlatmap();

}

void Calibrator::loadCalFlatmap()
{
    cv::Mat fullFlatmap = nullingFlatmap + speckleFlatmap;
    if(cfgParams.get<bool>("P3KParams.useCentoffs"))
        (*p3k).loadNewCentoffsFromFlatmap(fullFlatmap);

}

void Calibrator::unloadCalFlatmap()
{
    if(cfgParams.get<bool>("P3KParams.useCentoffs"))
        (*p3k).loadNewCentoffsFromFlatmap(nullingFlatmap);

}

void Calibrator::measureCalSpeckleIntensities()
{ 
    int speckleAperture = cfgParams.get<int>("NullingParams.apertureRadius");
    cv::Mat speckleIm;
    cv::Mat apertureMask = cv::Mat::zeros(2*cfgParams.get<int>("NullingParams.apertureRadius")+1, 2*cfgParams.get<int>("NullingParams.apertureRadius")+1, CV_64F);
    cv::circle(apertureMask, cv::Point(cfgParams.get<int>("NullingParams.apertureRadius"), cfgParams.get<int>("NullingParams.apertureRadius")), cfgParams.get<int>("NullingParams.apertureRadius"), 1, -1);
    
    for(int i=0; i<curKvecs.size(); i++)
    { 
        if(isValidPos[i]) 
        {
            speckleIm = cv::Mat(image, cv::Range((int)curTruePos[i].y-speckleAperture, (int)curTruePos[i].y+speckleAperture+1),
            cv::Range((int)curTruePos[i].x-speckleAperture, (int)curTruePos[i].x+speckleAperture+1));
            speckleIm = speckleIm.mul(apertureMask);
            posSpeckleIntensities[i] = (double)cv::sum(speckleIm)[0]/measureIntensityCorrection(curTruePos[i]);
            std::cout << posSpeckleIntensities[i] << std::endl;

        }

        if(isValidNeg[i]) 
        {
            speckleIm = cv::Mat(image, cv::Range((int)curTrueNeg[i].y-speckleAperture, (int)curTrueNeg[i].y+speckleAperture+1),
            cv::Range((int)curTrueNeg[i].x-speckleAperture, (int)curTrueNeg[i].x+speckleAperture+1));
            speckleIm = speckleIm.mul(apertureMask);
            negSpeckleIntensities[i] = (double)cv::sum(speckleIm)[0]/measureIntensityCorrection(curTruePos[i]);
            std::cout << negSpeckleIntensities[i] << std::endl;

        }

    }

}

double Calibrator::measureIntensityCorrection(cv::Point2d coords)
{
    cv::Mat goodPixMask = (~badPixMask)&1;
    cv::Mat apertureGoodPixMask = cv::Mat(goodPixMask, cv::Range((int)coords.y-cfgParams.get<int>("NullingParams.apertureRadius"), 
        (int)coords.y+cfgParams.get<int>("NullingParams.apertureRadius")+1), cv::Range(coords.x-cfgParams.get<int>("NullingParams.apertureRadius"), 
        coords.x+cfgParams.get<int>("NullingParams.apertureRadius")+1));
    apertureGoodPixMask.convertTo(apertureGoodPixMask, CV_64F);
    cv::Mat gaussKernel = cv::getGaussianKernel(2*cfgParams.get<int>("NullingParams.apertureRadius")+1, cfgParams.get<double>("ImgParams.lambdaOverD")*0.42);
    gaussKernel = gaussKernel*gaussKernel.t();
    return (double)cv::sum(gaussKernel.mul(apertureGoodPixMask))[0]/cv::sum(gaussKernel)[0];

}
