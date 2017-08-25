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
        int xCenter;
        int yCenter;
        int xCtrlStart;
        int xCtrlEnd;
        int yCtrlStart;
        int yCtrlEnd;

    public:
        ImageGrabberSim(int xCent=40, int yCent=60);
        ~ImageGrabberSim();
        void readImageData(std::string filename);
        cv::Mat& getImage();
        void displayImage(bool showControlRegion=false);
        void changeCenter(int xCent, int yCent);

    private:
        void getControlRegion();
        void setCtrlRegion();

};
#endif