#include "SpeckleNuller.h"

SpeckleNuller::SpeckleNuller()
{
    int ctrlRegionXSize = XCTRLEND - XCTRLSTART;
    int ctrlRegionYSize = YCTRLEND - YCTRLSTART;
    int ctrlRegionXOffs = XCTRLSTART - CENTERX;
    int ctrlRegionYOffs = YCTRLSTART - CENTERY;
    image.create(ctrlRegionYSize, ctrlRegionXSize, CV_16UC1);

}

void SpeckleNuller::updateImage()
{
    std::string filename = "/home/neelay/SpeckleNulling/DarknessSpeckleSuppression/darkness_simulation/images/14992057476.img";
    imgGrabber.readImageData(filename);
    image = imgGrabber.getImage();

}

void SpeckleNuller::detectSpeckles()
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
            maxImgPts.push_back(tempPt);

    }
    
    //Sort list of ImgPts
    std::sort(maxImgPts.begin(), maxImgPts.end());
    
    std::vector<ImgPt>::iterator curElem, kt;
    ImgPt curPt;
    double ptDist;
    curElem = maxImgPts.begin();
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

    // for(kt = maxImgPts.begin(); kt != maxImgPts.end(); kt++)
    // {
    //     std::cout << "coordinates" << (*kt).coordinates << std::endl;
    //     std::cout << "intenstiy" << (*kt).intensity << std::endl;
    //     std::cout << std::endl;

    // }
    //imgGrabber.displayImage(true);

}


