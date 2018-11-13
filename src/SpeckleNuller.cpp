#include "SpeckleNuller.h"

bool cmpImgPt(ImgPt lhs, ImgPt rhs)
{
    return lhs.intensity > rhs.intensity;

}

SpeckleNuller::SpeckleNuller(boost::property_tree::ptree &ptree, bool vbose)
{
    cfgParams = ptree;
    int ctrlRegionXSize = cfgParams.get<int>("ImgParams.xCtrlEnd") - cfgParams.get<int>("ImgParams.xCtrlStart");
    int ctrlRegionYSize = cfgParams.get<int>("ImgParams.yCtrlEnd") - cfgParams.get<int>("ImgParams.yCtrlStart");
    image.create(ctrlRegionYSize, ctrlRegionXSize, CV_64FC1);
    verbose = vbose;
    //imgGrabber = ImageGrabber(ptree);
    std::cout << "Creating P3K Object" << std::endl;
    p3k = new P3KComFast(ptree);
    if(cfgParams.get<bool>("P3KParams.useCentoffs"))
        (*p3k).grabCurrentCentoffs();
    else
        (*p3k).grabCurrentFlatmap();
    nullingFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64FC1);

}

/*
void SpeckleNuller::updateImage(uint64_t timestamp)
{
    imgGrabber.startIntegrating(timestamp);
    imgGrabber.readNextImage();
    image = imgGrabber.getCtrlRegionImage();
    //imgGrabber.displayImage(true);

}*/

void SpeckleNuller::updateImage(cv::Mat &newImage)
{
    image = newImage;

}

void SpeckleNuller::updateBadPixMask(cv::Mat &newMask)
{
    badPixMask = newMask;

}

void SpeckleNuller::updateCurFlatmap()
{}


std::vector<ImgPt> SpeckleNuller::detectSpeckles()
{   
    double usFactor = cfgParams.get<double>("NullingParams.usFactor");

    //first do gaussian us filt on image
    cv::Mat filtImg = gaussianBadPixUSFilt(image, badPixMask, (int)usFactor, cfgParams.get<double>("ImgParams.lambdaOverD"));

    //scale image parameters by usFactor, since image is upsampled
    int speckleWindow = cfgParams.get<int>("NullingParams.speckleWindow")*cfgParams.get<int>("NullingParams.usFactor");
    int apertureRadius = cfgParams.get<double>("NullingParams.apertureRadius");
    int kvecOffsSize = (int)cfgParams.get<double>("NullingParams.kvecOffsSize");

    //Find local maxima within cfgParams.get<int>("NullingParams.speckleWindow") size window
    cv::Mat kernel = cv::Mat::ones(speckleWindow, speckleWindow, CV_8UC1);
    cv::Mat maxFiltIm, isMaximum;
    std::vector<cv::Point2i> maxima;
    std::vector<cv::Point2i> speckleLocs;
    std::vector<ImgPt> maxImgPts;

    if(cfgParams.get<bool>("NullingParams.useBoxBlur"))
        cv::blur(filtImg.clone(), filtImg, cv::Size2i(speckleWindow, speckleWindow));
    
    if(cfgParams.get<bool>("NullingParams.useGaussianBlur"))
        cv::blur(filtImg.clone(), filtImg, cv::Size2i(speckleWindow, speckleWindow));

    cv::dilate(filtImg, maxFiltIm, kernel);
    cv::compare(filtImg, maxFiltIm, isMaximum, cv::CMP_EQ);
    cv::findNonZero(isMaximum, maxima); //maxima are coordinates in upsampled filtImg
    
    //Put Points in ImgPt Struct List
    std::vector<cv::Point2i>::iterator it;
    ImgPt tempPt;
    for(it = maxima.begin(); it != maxima.end(); it++)
    {
        tempPt.coordinates = cv::Point2d((double)(*it).x/usFactor, (double)(*it).y/usFactor); //coordinates in real image
        tempPt.intensity = filtImg.at<double>(*it);
        if(tempPt.intensity != 0)
            if((tempPt.coordinates.x < (image.cols-apertureRadius-kvecOffsSize)) && (tempPt.coordinates.x > (apertureRadius+kvecOffsSize))
                && (tempPt.coordinates.y < (image.rows-apertureRadius-kvecOffsSize)) && (tempPt.coordinates.y > (apertureRadius+kvecOffsSize)))
            maxImgPts.push_back(tempPt);

    }
    
    //Sort list of ImgPts
    std::sort(maxImgPts.begin(), maxImgPts.end(), cmpImgPt);

    if(verbose)
        std::cout << "SpeckleNuller: Detected " << maxImgPts.size() << " bright spots." << std::endl;
    
    
    //imgGrabber.displayImage(true);
    
    return maxImgPts;

}

void SpeckleNuller::exclusionZoneCut(std::vector<ImgPt> &maxImgPts)
{
    std::vector<ImgPt>::iterator curElem, kt;
    ImgPt curPt;
    double ptDist;
    bool curElemRemoved;
    int exclusionZone = cfgParams.get<int>("NullingParams.exclusionZone");

    for(curElem = maxImgPts.begin(); 
        (curElem < (maxImgPts.begin()+cfgParams.get<int>("NullingParams.maxSpeckles"))) && (curElem < maxImgPts.end()); curElem++)
    {
        curPt = *curElem;
        curElemRemoved = false;
        //Check to see if curPt is too close to any speckle, but only if we're keeping all currently active speckles
        if(!cfgParams.get<bool>("TrackingParams.enforceRedetection"))
        {
            std::vector<Speckle>::iterator speckIter;
            for(speckIter = specklesList.begin(); speckIter < specklesList.end(); speckIter++)
            {
                ptDist = cv::norm(curPt.coordinates - (*speckIter).getCoordinates());
                if(ptDist <= exclusionZone)
                {
                    maxImgPts.erase(curElem);
                    curElem--;
                    curElemRemoved = true;
                    break;

                }

            }

        }

        if(curElemRemoved)
            continue;

        //If not too close to speckle, remove other maxImgPts within exclusion zone
        for(kt = curElem+1; kt < maxImgPts.end(); kt++)
        {
            ptDist = cv::norm(curPt.coordinates - (*kt).coordinates);
            // std::cout << "curElem " << (*curElem).coordinates << std::endl;
            // std::cout << "maxImgPts0" << maxImgPts[0].coordinates << std::endl;
            // std::cout << "kt " << (*kt).coordinates << std::endl;
            // std::cout << "maxImgPtsend " << (*(maxImgPts.end()-1)).coordinates << std::endl;
            if(ptDist <= exclusionZone)
            {
                //std::cout << "pdist" << ptDist << std::endl;
                maxImgPts.erase(kt);
                kt--;

            }

        }

    }

    if(specklesList.size() >= cfgParams.get<int>("NullingParams.maxSpeckles"))
        maxImgPts.clear();
    
    else if(maxImgPts.size() > (cfgParams.get<int>("NullingParams.maxSpeckles") - specklesList.size()))
        maxImgPts.erase(maxImgPts.begin()+cfgParams.get<int>("NullingParams.maxSpeckles") - specklesList.size(), maxImgPts.end());


}

void SpeckleNuller::updateAndCutActiveSpeckles(std::vector<ImgPt> &maxImgPts)
{
    std::vector<ImgPt>::iterator ptIter;
    std::vector<Speckle>::iterator speckIter;
    bool speckFound; // check if nulled speckle has been detected
    int exclusionZone = cfgParams.get<int>("NullingParams.exclusionZone");
    double ptDist;
    
    for(speckIter = specklesList.begin(); speckIter < specklesList.end(); speckIter++)
    {
        speckFound = false;
        for(ptIter = maxImgPts.begin(); ptIter < maxImgPts.end(); ptIter++)
        {
            ptDist = cv::norm((*ptIter).coordinates - (*speckIter).getCoordinates());
            if(ptDist < cfgParams.get<double>("TrackingParams.distThresh"))
            {
                speckFound = true;
                if(cfgParams.get<bool>("TrackingParams.updateCoords"))
                    (*speckIter).setCoordinates((*ptIter).coordinates);
                (*speckIter).measureIntensityCorrection(badPixMask);
                (*speckIter).measureSpeckleIntensityAndSigma(image);
                if(verbose)
                    std::cout << "SpeckleNuller: Re-detected active speckle at " << (*ptIter).coordinates << std::endl;
                maxImgPts.erase(ptIter);
                break;

            }


        }

        if(!speckFound) // delete speckle from specklesList if it wasn't detected this iteration
        {
            specklesList.erase(speckIter);
            speckIter--;

        }

    }

}

void SpeckleNuller::updateAndCutNulledSpeckles(std::vector<ImgPt> &maxImgPts)
{
    std::vector<ImgPt>::iterator ptIter;
    std::vector<Speckle>::iterator speckIter;
    bool speckFound; // check if nulled speckle has been detected
    int exclusionZone = cfgParams.get<int>("NullingParams.exclusionZone");
    double ptDist;
    
    for(speckIter = nullSpecklesList.begin(); speckIter < nullSpecklesList.end(); speckIter++)
    {
        speckFound = false;
        for(ptIter = maxImgPts.begin(); ptIter < maxImgPts.end(); ptIter++)
        {
            ptDist = cv::norm((*ptIter).coordinates - (*speckIter).getCoordinates());
            if(ptDist < cfgParams.get<double>("TrackingParams.distThresh"))
            {
                speckFound = true;
                if(cfgParams.get<bool>("TrackingParams.updateCoords"))
                    (*speckIter).setCoordinates((*ptIter).coordinates);
                (*speckIter).updateNulledSpeckle();
                (*speckIter).measureIntensityCorrection(badPixMask);
                (*speckIter).measureSpeckleIntensityAndSigma(image);
                if(verbose)
                    std::cout << "SpeckleNuller: Re-detected nulled speckle at " << (*ptIter).coordinates << std::endl;
                specklesList.push_back(*speckIter);
                maxImgPts.erase(ptIter);
                break;

            }

        }

    }

    nullSpecklesList.clear();

}
            
void SpeckleNuller::createSpeckleObjects(std::vector<ImgPt> &imgPts)
{ 
    if(verbose)
        std::cout << "SpeckleNuller: creating speckle objects..." << std::endl;

    std::vector<ImgPt>::iterator it;
    cv::Point2d coordinates;
    for(it = imgPts.begin(); it < imgPts.end(); it++)
    {
        coordinates = (*it).coordinates;
        Speckle speck = Speckle(coordinates, cfgParams);
        speck.measureIntensityCorrection(badPixMask);
        speck.measureSpeckleIntensityAndSigma(image);
        specklesList.push_back(speck);
        if(verbose)
        {
            std::cout << "Coordinates: " << coordinates << std::endl;
            std::cout << "Intensity: " << speck.getIntensity() << std::endl;

        }

    }

    if(verbose)
        std::cout << std::endl;

}

void SpeckleNuller::updateExistingSpeckles()
{
    std::vector<Speckle>::iterator it;
    
    for(it = specklesList.begin(); it < specklesList.end(); it++)
    {
        (*it).measureSpeckleIntensityAndSigma(image);

    }

}
    

void SpeckleNuller::measureSpeckleProbeIntensities(int phaseInd)
{
    std::vector<Speckle>::iterator it;
    
    if(verbose)
        std::cout << "SpeckleNuller: measuring probe intensities (index " << phaseInd << ")..." << std::endl; 

    for(it = specklesList.begin(); it < specklesList.end(); it++)
    {
        (*it).measurePhaseIntensity(phaseInd, image);

    }

}

void SpeckleNuller::measureSpeckleGainIntensities(int gainInd)
{
    std::vector<Speckle>::iterator it;
    
    if(verbose)
        std::cout << "SpeckleNuller: measuring probe gain intensities (index " << gainInd << ")..." << std::endl; 

    for(it = specklesList.begin(); it < specklesList.end(); it++)
    {
        (*it).measureGainIntensity(gainInd, image);

    }

}

void SpeckleNuller::updateSpecklesAndCheckNull()
{
    std::vector<Speckle>:: iterator speckIter;

    if(verbose) 
        std::cout << "SpeckleNuller: updating speckle states" << std::endl << std::endl;

    for(speckIter = specklesList.begin(); speckIter < specklesList.end(); speckIter++)
    {
        (*speckIter).updateStateEstimates();
        if((*speckIter).checkToNull())
        {
            (*speckIter).calculateFinalPhase();
            if(!cfgParams.get<bool>("NullingParams.useGainLoop"))
                (*speckIter).calculateFinalGain();
            nullSpecklesList.push_back(*speckIter);
            if(verbose)
            {
                std::cout << "Nulling speckle at " << (*speckIter).getCoordinates() << std::endl;
                std::cout << "final phase: " << (*speckIter).getFinalPhase() << std::endl;
                std::cout << "final gain: " << (*speckIter).getFinalGain() << std::endl;
                std::cout << std::endl;

            }
            specklesList.erase(speckIter);
            speckIter--;

        }

    }

    if(verbose)
        std::cout << std::endl;

}
            
        

void SpeckleNuller::calculateFinalPhases()
{
    std::vector<Speckle>::iterator it;
    
    if(verbose)
        std::cout << "SpeckleNuller: calculating final phases..." << std::endl;

    for(it = specklesList.begin(); it < specklesList.end(); it++)
    {
        (*it).calculateFinalPhase();
        if(verbose)
        {
            std::cout << "Coordinates: " << (*it).getCoordinates();
            std::cout << " Final phase: " << (*it).getFinalPhase() << std::endl;

        }

    }

}

void SpeckleNuller::calculateFinalGains()
{
    std::vector<Speckle>::iterator it;
    
    if(verbose)
        std::cout << "SpeckleNuller: calculating final gains..." << std::endl;

    for(it = nullSpecklesList.begin(); it < nullSpecklesList.end(); it++)
    {
        (*it).calculateFinalGain();
        if(verbose)
        {
            std::cout << "Coordinates: " << (*it).getCoordinates();
            std::cout << " Final gain: " << (*it).getFinalGain() << std::endl;

        }

    }

}

void SpeckleNuller::generateProbeFlatmap(std::vector<int> &phaseInds)
{
    std::vector<Speckle>::iterator it;
    std::vector<int>::iterator indIt = phaseInds.begin();
    //probeFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);
    probeFlatmap = nullingFlatmap.clone();

    for(it = specklesList.begin(); it < specklesList.end(); it++)
    {
        probeFlatmap += (*it).getProbeSpeckleFlatmap(*indIt);
        indIt++;

    }

}

void SpeckleNuller::generateProbeFlatmap(int phaseInd)
{
    std::vector<Speckle>::iterator it;
    //probeFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);
    probeFlatmap = nullingFlatmap.clone();

    for(it = specklesList.begin(); it < specklesList.end(); it++)
    {
        probeFlatmap += (*it).getProbeSpeckleFlatmap(phaseInd);

    }

}

void SpeckleNuller::generateProbeGainFlatmap(int gainInd)
{
    std::vector<Speckle>::iterator it;
    //probeFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);
    probeFlatmap = nullingFlatmap.clone();

    for(it = nullSpecklesList.begin(); it < nullSpecklesList.end(); it++)
    {
        probeFlatmap += (*it).getProbeGainSpeckleFlatmap(gainInd);

    }

}

void SpeckleNuller::loadProbeSpeckles()
{
    if(cfgParams.get<bool>("P3KParams.useCentoffs"))
        (*p3k).loadNewCentoffsFromFlatmap(probeFlatmap);

}

void SpeckleNuller::loadNullingSpeckles()
{
    if(cfgParams.get<bool>("P3KParams.useCentoffs"))
        (*p3k).loadNewCentoffsFromFlatmap(nullingFlatmap);

}

void SpeckleNuller::generateNullingFlatmap()
{
    std::vector<Speckle>::iterator it;
    //nullingFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);
    if(verbose)
        std::cout << "Generating Nulling Flatmap using " << nullSpecklesList.size() << " speckles" << std::endl;
    for(it = nullSpecklesList.begin(); it < nullSpecklesList.end(); it++) 
        nullingFlatmap += (*it).getFinalSpeckleFlatmap();
            
}

void SpeckleNuller::generateSimProbeSpeckles(int phaseInd)
{
    std::vector<Speckle>::iterator it;

    for(it = specklesList.begin(); it < specklesList.end(); it++)
    {
        (*it).generateSimProbeSpeckle(phaseInd);

    }

}

void SpeckleNuller::generateSimFinalSpeckles()
{
    std::vector<Speckle>::iterator it;
    if(nullSpecklesList.size()==0)
        writeToKvecFile(cv::Point2d(0,0), 0, 0, 1);

    for(it = nullSpecklesList.begin(); it < nullSpecklesList.end(); it++)
    {
        (*it).generateSimFinalSpeckle();

    }

}

void SpeckleNuller::clearSpeckleObjects() {specklesList.clear();}
