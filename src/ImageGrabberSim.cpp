#include "ImageGrabberSim.h"

ImageGrabberSim::ImageGrabberSim(int xCent, int yCent)
{
    imageArr = new char[2*IMXSIZE*IMYSIZE];
    xCenter = xCent;
    yCenter = yCent;
    setCtrlRegion();

}

ImageGrabberSim::~ImageGrabberSim()
{
    delete imageArr;

}

void ImageGrabberSim::readImageData(std::string filename)
{
    std::ifstream imgFile(filename.c_str(), std::ifstream::in|std::ifstream::binary);
    imgFile.read(imageArr, 2*IMXSIZE*IMYSIZE);
    cv::Mat rawImageTp = cv::Mat(IMXSIZE, IMYSIZE, CV_16UC1, imageArr);
    cv::transpose(rawImageTp, rawImage);
    getControlRegion();

}

void ImageGrabberSim::getControlRegion()
{
    ctrlRegionImage = cv::Mat(rawImage, cv::Range(yCtrlStart, yCtrlEnd), cv::Range(xCtrlStart, xCtrlEnd));

}

cv::Mat &ImageGrabberSim::getImage()
{
    return ctrlRegionImage;

}

void ImageGrabberSim::setCtrlRegion()
{
    xCtrlStart = xCenter + XCTRLSTART;
    xCtrlEnd = xCenter + XCTRLEND;
    yCtrlStart = yCenter + YCTRLSTART;
    yCtrlEnd = yCenter + YCTRLEND;

}

void ImageGrabberSim::displayImage(bool showControlRegion)
{
    std::cout << "rawImageCtrl " << ctrlRegionImage << std::endl;
    cv::namedWindow("DARKNESS Sim Image", cv::WINDOW_NORMAL);
    if(showControlRegion)
        cv::imshow("DARKNESS Sim Image", 2000*ctrlRegionImage);
    else
        cv::imshow("DARKNESS Sim Image", 2000*rawImage);
    cv::waitKey(0);

}

