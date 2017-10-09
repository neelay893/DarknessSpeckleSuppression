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
    image.create(ctrlRegionYSize, ctrlRegionXSize, CV_16UC1);
    verbose = vbose;
    imgGrabber = ImageGrabber(ptree);
    std::cout << "Creating P3K Object" << std::endl;
    p3k = new P3KCom(ptree);
    if(cfgParams.get<bool>("P3KParams.useCentoffs"))
        (*p3k).grabCurrentCentoffs();
    else
        (*p3k).grabCurrentFlatmap();
    nullingFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64FC1);

}

void SpeckleNuller::updateImage(uint64_t timestamp)
{
    imgGrabber.startIntegrating(timestamp);
    imgGrabber.readNextImage();
    image = imgGrabber.getCtrlRegionImage();
    //imgGrabber.displayImage(true);

}

void SpeckleNuller::updateCurFlatmap()
{}


std::vector<ImgPt> SpeckleNuller::detectSpeckles()
{
    //Find local maxima within cfgParams.get<int>("NullingParams.speckleWindow") size window
    cv::Mat kernel = cv::Mat::ones(cfgParams.get<int>("NullingParams.speckleWindow"), cfgParams.get<int>("NullingParams.speckleWindow"), CV_8UC1);
    cv::Mat maxFiltIm, isMaximum;
    std::vector<cv::Point2i> maxima;
    std::vector<cv::Point2i> speckleLocs;
    std::vector<ImgPt> maxImgPts;

    if(cfgParams.get<bool>("NullingParams.useBoxBlur"))
        cv::blur(image.clone(), image, cv::Size2i(cfgParams.get<int>("NullingParams.speckleWindow"), cfgParams.get<int>("NullingParams.speckleWindow")));
    
    if(cfgParams.get<bool>("NullingParams.useGaussianBlur"))
        cv::blur(image.clone(), image, cv::Size2i(cfgParams.get<int>("NullingParams.speckleWindow"), cfgParams.get<int>("NullingParams.speckleWindow")));

    cv::dilate(image, maxFiltIm, kernel);
    cv::compare(image, maxFiltIm, isMaximum, cv::CMP_EQ);
    cv::findNonZero(isMaximum, maxima);
    
    //Put Points in ImgPt Struct List
    std::vector<cv::Point2i>::iterator it;
    ImgPt tempPt;
    for(it = maxima.begin(); it != maxima.end(); it++)
    {
        tempPt.coordinates = *it;
        tempPt.intensity = image.at<ushort>(*it);
        if(tempPt.intensity != 0)
            if((tempPt.coordinates.x < (image.cols-cfgParams.get<int>("NullingParams.apertureRadius"))) && (tempPt.coordinates.x > cfgParams.get<int>("NullingParams.apertureRadius"))
                && (tempPt.coordinates.y < (image.rows-cfgParams.get<int>("NullingParams.apertureRadius"))) && (tempPt.coordinates.y > cfgParams.get<int>("NullingParams.apertureRadius")))
            maxImgPts.push_back(tempPt);

    }
    
    //Sort list of ImgPts
    std::sort(maxImgPts.begin(), maxImgPts.end(), cmpImgPt);
    
    std::vector<ImgPt>::iterator curElem, kt;
    ImgPt curPt;
    double ptDist;

    for(curElem = maxImgPts.begin(); 
        (curElem < (maxImgPts.begin()+cfgParams.get<int>("NullingParams.maxSpeckles"))) && (curElem < maxImgPts.end()); curElem++)
    {
        curPt = *curElem;
        for(kt = curElem+1; kt < maxImgPts.end(); kt++)
        {
            ptDist = norm(curPt.coordinates - (*kt).coordinates);
            // std::cout << "curElem " << (*curElem).coordinates << std::endl;
            // std::cout << "maxImgPts0" << maxImgPts[0].coordinates << std::endl;
            // std::cout << "kt " << (*kt).coordinates << std::endl;
            // std::cout << "maxImgPtsend " << (*(maxImgPts.end()-1)).coordinates << std::endl;
            if(ptDist <= cfgParams.get<int>("NullingParams.exclusionZone"))
            {
                //std::cout << "pdist" << ptDist << std::endl;
                maxImgPts.erase(kt);
                kt--;

            }

        }

    }
    
    if(maxImgPts.size() > cfgParams.get<int>("NullingParams.maxSpeckles"))
        maxImgPts.erase(maxImgPts.begin()+cfgParams.get<int>("NullingParams.maxSpeckles"), maxImgPts.end());


    if(verbose)
    {
        for(kt = maxImgPts.begin(); kt != maxImgPts.end(); kt++)
        {
            std::cout << "coordinates" << (*kt).coordinates << std::endl;
            std::cout << "intenstiy" << (*kt).intensity << std::endl;
            std::cout << std::endl;

        }

    }
    
    //imgGrabber.displayImage(true);
    
    return maxImgPts;

}

void SpeckleNuller::createSpeckleObjects(std::vector<ImgPt> &imgPts)
{
    std::vector<ImgPt>::iterator it;
    
    if(verbose)
        std::cout << "SpeckleNuller: creating speckle objects..." << std::endl;

    cv::Point2i coordinates;
    for(it = imgPts.begin(); it < imgPts.end(); it++)
    {
        coordinates = (*it).coordinates;
        std::cout << "Old Coordinates " << coordinates << std::endl;
        if(cfgParams.get<bool>("NullingParams.applyRandomCoordinateOffset"))
        {
            coordinates.x += rand()%3-1;
            coordinates.y += rand()%3-1;
            std::cout << "New Coordinates " << coordinates << std::endl;

        }

        Speckle speck = Speckle(coordinates, cfgParams);
        speck.setInitialIntensity(speck.measureSpeckleIntensity(image));
        specklesList.push_back(speck);
        if(verbose)
        {
            std::cout << "Coordinates: " << coordinates << std::endl;
            std::cout << " Intensity: " << speck.measureSpeckleIntensity(image) << std::endl;

        }

    }

}

void SpeckleNuller::measureSpeckleProbeIntensities(int phaseInd)
{
    std::vector<Speckle>::iterator it;
    
    if(verbose)
        std::cout << "SpeckleNuller: measuring probe intensities (index " << phaseInd << ")..." << std::endl; 

    for(it = specklesList.begin(); it < specklesList.end(); it++)
    {
        (*it).incrementPhaseIntensity(phaseInd, (*it).measureSpeckleIntensity(image));
        if(verbose)
        {
            std::cout << "Coordinates: " << (*it).getCoordinates();
            std::cout << " Intensity: " << (*it).measureSpeckleIntensity(image) << std::endl;

        }

    }

}

void SpeckleNuller::measureSpeckleGainIntensities(int gainInd)
{
    std::vector<Speckle>::iterator it;
    
    if(verbose)
        std::cout << "SpeckleNuller: measuring probe intensities (index " << gainInd << ")..." << std::endl; 

    for(it = specklesList.begin(); it < specklesList.end(); it++)
    {
        (*it).incrementGainIntensity(gainInd, (*it).measureSpeckleIntensity(image));
        if(verbose)
        {
            std::cout << "Coordinates: " << (*it).getCoordinates();
            std::cout << " Intensity: " << (*it).measureSpeckleIntensity(image) << std::endl;

        }

    }

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

    for(it = specklesList.begin(); it < specklesList.end(); it++)
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
    probeFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);

    for(it = specklesList.begin(); it < specklesList.end(); it++)
    {
        probeFlatmap += (*it).getProbeSpeckleFlatmap(*indIt);
        indIt++;

    }

}

void SpeckleNuller::generateProbeFlatmap(int phaseInd)
{
    std::vector<Speckle>::iterator it;
    probeFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);

    for(it = specklesList.begin(); it < specklesList.end(); it++)
    {
        probeFlatmap += (*it).getProbeSpeckleFlatmap(phaseInd);

    }

}

void SpeckleNuller::generateProbeGainFlatmap(int gainInd)
{
    std::vector<Speckle>::iterator it;
    probeFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);

    for(it = specklesList.begin(); it < specklesList.end(); it++)
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

void SpeckleNuller::generateNullingFlatmap(double gain)
{
    std::vector<Speckle>::iterator it;
    //nullingFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);

    for(it = specklesList.begin(); it < specklesList.end(); it++)
    {
        nullingFlatmap += (*it).getFinalSpeckleFlatmap(gain);

    }

}

void SpeckleNuller::generateNullingFlatmap()
{
    std::vector<Speckle>::iterator it;
    //nullingFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);
    
    for(it = specklesList.begin(); it < specklesList.end(); it++)
    {
        if(cfgParams.get<bool>("NullingParams.useGainLoop"))
        {
            nullingFlatmap += (*it).getFinalSpeckleFlatmap((*it).getFinalGain());
            if(verbose)
            {
                std::cout << "Coordinates: " << (*it).getCoordinates() << std::endl;
                std::cout << "Final Gain: " << (*it).getFinalGain() << std::endl;

            }

        }

        else
            nullingFlatmap += (*it).getFinalSpeckleFlatmap(cfgParams.get<double>("NullingParams.defaultGain"));
            

    }

}

void SpeckleNuller::generateSimProbeSpeckles(int phaseInd)
{
    std::vector<Speckle>::iterator it;

    for(it = specklesList.begin(); it < specklesList.end(); it++)
    {
        (*it).generateSimProbeSpeckle(phaseInd);

    }

}

void SpeckleNuller::generateSimFinalSpeckles(double gain)
{
    std::vector<Speckle>::iterator it;

    for(it = specklesList.begin(); it < specklesList.end(); it++)
    {
        (*it).generateSimFinalSpeckle(gain);

    }

}

void SpeckleNuller::clearSpeckleObjects() {specklesList.clear();}
