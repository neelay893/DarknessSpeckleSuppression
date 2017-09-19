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
            if((tempPt.coordinates.x < (image.cols-cfgParams.get<int>("NullingParams.apertureRadius"))) && (tempPt.coordinates.x >= cfgParams.get<int>("NullingParams.apertureRadius"))
                && (tempPt.coordinates.y < (image.rows-cfgParams.get<int>("NullingParams.apertureRadius"))) && (tempPt.coordinates.y >= cfgParams.get<int>("NullingParams.apertureRadius")))
            maxImgPts.push_back(tempPt);

    }
    
    //Sort list of ImgPts
    std::sort(maxImgPts.begin(), maxImgPts.end(), cmpImgPt);
    
    std::vector<ImgPt>::iterator curElem, kt;
    ImgPt curPt;
    double ptDist;

    for(curElem = maxImgPts.begin(); 
        (curElem < (maxImgPts.begin()+MAXSPECKLES)) && (curElem < maxImgPts.end()); curElem++)
    {
        curPt = *curElem;
        for(kt = curElem+1; kt < maxImgPts.end(); kt++)
        {
            ptDist = norm(curPt.coordinates - (*kt).coordinates);
            // std::cout << "curElem " << (*curElem).coordinates << std::endl;
            // std::cout << "maxImgPts0" << maxImgPts[0].coordinates << std::endl;
            // std::cout << "kt " << (*kt).coordinates << std::endl;
            // std::cout << "maxImgPtsend " << (*(maxImgPts.end()-1)).coordinates << std::endl;
            if(ptDist <= EXCLUSIONZONE)
            {
                //std::cout << "pdist" << ptDist << std::endl;
                maxImgPts.erase(kt);
                kt--;

            }

        }

    }
    
    if(maxImgPts.size() > 16)
        maxImgPts.erase(maxImgPts.begin()+16, maxImgPts.end());


    if(verbose)
    {
        for(kt = maxImgPts.begin(); kt != maxImgPts.end(); kt++)
        {
            std::cout << "coordinates" << (*kt).coordinates << std::endl;
            std::cout << "intenstiy" << (*kt).intensity << std::endl;
            std::cout << std::endl;

        }

    }
    
    imgGrabber.displayImage(true);
    
    return maxImgPts;

}

void SpeckleNuller::createSpeckleObjects(std::vector<ImgPt> &imgPts)
{
    std::vector<ImgPt>::iterator it;
    
    if(verbose)
        std::cout << "SpeckleNuller: creating speckle objects..." << std::endl;

    for(it = imgPts.begin(); it < imgPts.end(); it++)
    {
        Speckle speck = Speckle((*it).coordinates, cfgParams);
        speck.setInitialIntensity(speck.measureSpeckleIntensity(image));
        specklesList.push_back(speck);
        if(verbose)
        {
            std::cout << "Coordinates: " << (*it).coordinates;
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

void SpeckleNuller::generateProbeFlatmap(std::vector<int> &phaseInds)
{
    std::vector<Speckle>::iterator it;
    std::vector<int>::iterator indIt = phaseInds.begin();
    nextFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);

    for(it = specklesList.begin(); it < specklesList.end(); it++)
    {
        nextFlatmap += (*it).getProbeSpeckleFlatmap(*indIt);
        indIt++;

    }

}

void SpeckleNuller::generateProbeFlatmap(int phaseInd)
{
    std::vector<Speckle>::iterator it;
    nextFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);

    for(it = specklesList.begin(); it < specklesList.end(); it++)
    {
        nextFlatmap += (*it).getProbeSpeckleFlatmap(phaseInd);

    }

}

void SpeckleNuller::generateNullingFlatmap(double gain)
{
    std::vector<Speckle>::iterator it;
    nextFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);

    for(it = specklesList.begin(); it < specklesList.end(); it++)
    {
        nextFlatmap += (*it).getFinalSpeckleFlatmap(gain);

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
