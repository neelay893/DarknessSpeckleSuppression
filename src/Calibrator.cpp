#include "Calibrator.h"

Calibrator::Calibrator(boost::property_tree::ptree &ptree, P3KCom *p)
{
    p3k = p;
    cfgParams = ptree;
    nullingFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);
    lambdaOverD = cfgParams.get<double>("ImgParams.lambdaOverD");
    speckleFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);


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

cv::Point2d Calibrator::determineImgCenter()
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
        imCent = imCent/(double)nKvecsUsed;
    else
        imCent = cv::Point2d(-1, -1); //return this if no valid center was calculated

    return imCent;

}

void Calibrator::centroidCalSpeckles()
{ 
    for(int i=0; i<curKvecs.size(); i++)
    {
        if(isValidPos[i])
            curTruePos[i] = centroidPoint(curPosEstimates[i]);
        if(isValidNeg[i])
            curTrueNeg[i] = centroidPoint(curNegEstimates[i]);

    }

}
    


cv::Point2d Calibrator::centroidPoint(cv::Point2d posEstimate)
{
    int centroidingWindow = cfgParams.get<int>("ImgParams.usFactor")*cfgParams.get<int>("CalParams.centroidingWindowRadius");
    posEstimate = 2*posEstimate;
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
    coords.x = std::cos(dmAngle)*kvec.x - std::sin(dmAngle)*kvec.y;
    coords.y = std::sin(dmAngle)*kvec.x + std::cos(dmAngle)*kvec.y;
    return coords;

}


void Calibrator::addCalSpeckle(cv::Point2d kvec, double amplitude)
{
    curKvecs.push_back(kvec);
    dmAmplitudes.push_back(amplitude);
    cv::Point2d posEst = calculatePositionEstimate(kvec);
    curPosEstimates.push_back(posEst);
    curNegEstimates.push_back(-1*posEst);
    curTruePos.push_back(cv::Point2d(-1,-1));
    curTrueNeg.push_back(cv::Point2d(-1,-1));

    if((posEst.x >= 0) && (posEst.x < IMXSIZE) && (posEst.y >= 0) && (posEst.y < IMYSIZE))
        isValidPos.push_back(true);
    else
        isValidPos.push_back(false);

    if((-1*posEst.x >= 0) && (-1*posEst.x < IMXSIZE) && (-1*posEst.y >= 0) && (-1*posEst.y < IMYSIZE))
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
