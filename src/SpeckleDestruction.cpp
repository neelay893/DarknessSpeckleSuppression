#include <SpeckleNuller.h>
#include <ImageGrabberSim.h>
#include <iostream>

SpeckleNuller speckNull;

void speckleDetectLoop()
{
    for(int i=0; i<100; i++)
        speckNull.detectSpeckles();

}

int main()
{
    //SpeckleNuller speckNull;
    speckNull.updateImage();
    //speckNull.detectSpeckles();
    speckleDetectLoop();
    //ImageGrabberSim imgGrabber;
    //std::string filename = "/home/neelay/SpeckleNulling/DarknessSpeckleSuppression/darkness_simulation/images/14992057476.img";
    //imgGrabber.readImageData(filename);
    //imgGrabber.displayImage(true);
    //speckNull.updateImage();
    //speckNull.detectSpeckles();
    return 0;

}

