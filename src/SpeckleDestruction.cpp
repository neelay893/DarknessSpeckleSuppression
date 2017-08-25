#include <SpeckleNuller.h>
#include <Speckle.h>
#include <ImageGrabberSim.h>
#include <iostream>
#include <opencv2/opencv.hpp>

SpeckleNuller speckNull;

void speckleDetectLoop()
{
    for(int i=0; i<100; i++)
        speckNull.detectSpeckles();

}

int main()
{
    //SpeckleNuller speckNull;
    std::cout << "blah" << std::endl;
    
    for(int i=0; i<100; i++)
    {
        speckNull.updateImage();
        std::vector<ImgPt> rawSpecks = speckNull.detectSpeckles();
        speckNull.createSpeckleObjects(rawSpecks);
        speckNull.generateProbeFlatmap(1);
    
    }
    //cv::Point2i pt(5,2);
    //Speckle speck(pt);
    //speckNull.detectSpeckles();
    //speckleDetectLoop();
    //ImageGrabberSim imgGrabber;
    //std::string filename = "/home/neelay/SpeckleNulling/DarknessSpeckleSuppression/darkness_simulation/images/14992057476.img";
    //imgGrabber.readImageData(filename);
    //imgGrabber.displayImage(true);
    //speckNull.updateImage();
    //speckNull.detectSpeckles();
    return 0;

}

