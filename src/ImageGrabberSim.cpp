#include "ImageGrabberSim.h"

ImageGrabberSim::ImageGrabberSim()
{
    imageArr = new char[2*IMXSIZE*IMYSIZE];

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
    ctrlRegionImage = cv::Mat(rawImage, cv::Range(YCTRLSTART, YCTRLEND), cv::Range(XCTRLSTART, XCTRLEND));

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

cv::Mat &ImageGrabberSim::getImage()
{
    return ctrlRegionImage;

}
