#include "SpeckleNuller.h"

SpeckleNuller::SpeckleNuller()
{
    int ctrlRegionXSize = XCTRLEND - XCTRLSTART;
    int ctrlRegionYSize = YCTRLEND - YCTRLSTART;
    image.create(ctrlRegionYSize, ctrlRegionXSize, CV_16UC1);

}

void SpeckleNuller::updateImage()
{
    std::string filename = "/home/neelay/SpeckleNulling/DarknessSpeckleSuppression/darkness_simulation/images/14992057476.img";
    imgGrabber.startIntegrating(0);
    imgGrabber.readNextImage();
    image = imgGrabber.getCtrlRegionImageShm();
    //imgGrabber.displayImage(true);

}

std::vector<ImgPt> SpeckleNuller::detectSpeckles()
{
    //Find local maxima within SPECKLEWINDOW size window
    cv::Mat kernel = cv::Mat::ones(SPECKLEWINDOW, SPECKLEWINDOW, CV_8UC1);
    cv::Mat maxFiltIm, isMaximum;
    std::vector<cv::Point2i> maxima;
    std::vector<cv::Point2i> speckleLocs;
    std::vector<ImgPt> maxImgPts;

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
            if((tempPt.coordinates.x < (image.cols-SPECKLEAPERTURERADIUS)) && (tempPt.coordinates.x >= SPECKLEAPERTURERADIUS)
                && (tempPt.coordinates.y < (image.rows-SPECKLEAPERTURERADIUS)) && (tempPt.coordinates.y >= SPECKLEAPERTURERADIUS))
            maxImgPts.push_back(tempPt);

    }
    
    //Sort list of ImgPts
    std::sort(maxImgPts.begin(), maxImgPts.end());
    
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


    for(kt = maxImgPts.begin(); kt != maxImgPts.end(); kt++)
    {
        std::cout << "coordinates" << (*kt).coordinates << std::endl;
        std::cout << "intenstiy" << (*kt).intensity << std::endl;
        std::cout << std::endl;

    }
    //imgGrabber.displayImage(true);
    
    return maxImgPts;

}

void SpeckleNuller::createSpeckleObjects(std::vector<ImgPt> &imgPts)
{
    std::vector<ImgPt>::iterator it;

    for(it = imgPts.begin(); it < imgPts.end(); it++)
    {
        Speckle speck = Speckle((*it).coordinates);
        speck.setInitialIntensity(speck.measureSpeckleIntensity(image));
        specklesList.push_back(speck);

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
