#include <opencv2/opencv.hpp>
#include <iostream>
#include <params.h>

#ifndef IMAGEGRABBERSIM_H
#define IMAGEGRABBERSIM_H
class ImageGrabberSim
{
    private:
        cv::Mat rawImage;
        cv::Mat ctrlRegionImage;
        char *imageArr;

    public:
        ImageGrabberSim();
        ~ImageGrabberSim();
        void readImageData(std::string filename);
        cv::Mat& getImage();
        void displayImage(bool showControlRegion=false);

    private:
        void getControlRegion();

};
#endif
