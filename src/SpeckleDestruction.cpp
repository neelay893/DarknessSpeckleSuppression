#include <SpeckleNuller.h>
#include <Speckle.h>
#include <ImageGrabberSim.h>
#include <ImageGrabber.h>
#include <iostream>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <simInterfaceTools.h>
#include <chrono>

int main()
{
    //SpeckleNuller speckNull;

    /*
    std::cout << "blah" << std::endl;
    SpeckleNuller speckNull(true);
    std::vector<ImgPt> imgPts;

    while(1)
    {
         speckNull.updateImage();
         std::cout << "Detecting Speckles..." << std::endl;
         imgPts = speckNull.detectSpeckles();
         std::cout << "Creating Speckle Objects..." << std::endl;
         speckNull.createSpeckleObjects(imgPts);
         std::cout << "Creating Probe Speckles..." << std::endl;
         for(int i=0; i<4; i++)
         {
            speckNull.generateProbeFlatmap(i);
            speckNull.generateSimProbeSpeckles(i);
            writeDoneKvecFile();
            speckNull.updateImage();
            speckNull.measureSpeckleProbeIntensities(i);

         }

         std::cout << "Nulling Speckles..." << std::endl;
         speckNull.calculateFinalPhases();
         speckNull.generateNullingFlatmap(0.5);
         speckNull.generateSimFinalSpeckles(0.5);
         writeDoneKvecFile();
         speckNull.clearSpeckleObjects();
         if(access("QUIT", F_OK)!=-1)
            break;

    }
    */

    ImageGrabber imgGrabber;
    std::chrono::microseconds rawTime;
    uint64_t timestamp;
    while(1)
    {
        std::string dummy;
        std::cout << "Press any key...";
        std::getline(std::cin, dummy);
        std::cout << std::endl;
        rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
        timestamp = rawTime.count()/500;
        std::cout << "Raw TS: " << timestamp << std::endl;
        imgGrabber.startIntegrating(timestamp);
        imgGrabber.readNextImage();
        imgGrabber.displayImage();

    }
        



       

    
    // for(int i=0; i<100; i++)
    // {
    //     speckNull.updateImage();
    //     std::vector<ImgPt> rawSpecks = speckNull.detectSpeckles();
    //     speckNull.createSpeckleObjects(rawSpecks);
    //     speckNull.generateProbeFlatmap(1);
    // 
    // }
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

