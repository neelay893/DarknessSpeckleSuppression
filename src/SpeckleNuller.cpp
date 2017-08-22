#include "SpeckleNuller.h"

using namespace cv;
using namespace std;

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
    image = imread("/home/neelay/Pictures/m1_sim_plot_v2.png");
    namedWindow("Display", WINDOW_AUTOSIZE);
    imshow("Display", image);
    waitKey(0);

}

void SpeckleNuller::detectSpeckles()
{
    //Find local maxima within SPECKLEWINDOW size window
    Mat kernel = Mat::ones(SPECKLEWINDOW, SPECKLEWINDOW, CV_8UC1);
    Mat maxFiltIm, isMaximum;
    vector<Point2i> maxima;
    vector<Point2i> speckleLocs;
    vector<ImgPt> maxImgPts;

    dilate(image, maxFiltIm, kernel);
    compare(image, maxFiltIm, isMaximum, CMP_EQ);
    findNonZero(isMaximum, maxima);
    
    //Put Points in ImgPt Struct List
    vector<Point2i>::iterator it;
    ImgPt tempPt;
    for(it = maxima.begin(); it != maxima.end(); it++)
    {
        tempPt.coordinates = *it;
        tempPt.intensity = image.at<int>(*it);
        maxImgPts.push_back(tempPt);

    }
    
    //Sort list of ImgPts
    std::sort(maxImgPts.begin(), maxImgPts.end());
    
    vector<ImgPt>::iterator curElem, kt;
    ImgPt curPt;
    bool isDone = false;
    double ptDist;
    curElem = maxImgPts.begin();
    int idx;
    for(idx = 0; idx <MAXSPECKLES; idx++)
    {
        isDone = true;
        curPt = *curElem;
        for(kt = curElem+1; kt != maxImgPts.end(); kt++)
        {
            ptDist = norm(curPt.coordinates - (*kt).coordinates);
            cout << "curElem" << (*curElem).coordinates << endl;
            if(ptDist <= EXCLUSIONZONE)
            {
                maxImgPts.erase(kt);
                kt--;
                isDone = false;

            }

        }
        if(isDone)
            break;

    }

    for(kt = maxImgPts.begin(); kt != maxImgPts.end(); kt++)
    {
        cout << "coordinates" << (*kt).coordinates << endl;
        cout << "intenstiy" << (*kt).intensity << endl;
        cout << endl;

    }

}


