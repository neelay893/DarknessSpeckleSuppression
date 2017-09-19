#include <SpeckleNuller.h>
#include <Speckle.h>
#include <ImageGrabberSim.h>
#include <ImageGrabber.h>
#include <iostream>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <simInterfaceTools.h>
#include <chrono>

void speckNullSimLoop()
{
    boost::property_tree::ptree cfgParams;
    read_info("speckNullConfig.info", cfgParams);
    SpeckleNuller speckNull(cfgParams, true);
    std::vector<ImgPt> imgPts;

    while(1)
    {
         speckNull.updateImage(0);
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
            speckNull.updateImage(0);
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

}

void realImgGrabTest()
{ 
    boost::property_tree::ptree cfgParams;
    read_info("speckNullConfig.info", cfgParams);
    ImageGrabber imgGrabber(cfgParams);
    std::chrono::microseconds rawTime;
    uint64_t timestamp;
    while(1)
    {
        std::string dummy;
        std::cout << "Press any key...";
        std::getline(std::cin, dummy);
        std::cout << std::endl;
        rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
        timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
        std::cout << "Raw TS: " << timestamp << std::endl;
        std::cout << "Starting Integration..." << std::endl;
        imgGrabber.startIntegrating(timestamp);
        imgGrabber.readNextImage();
        std::cout << "Displaying Image..." << std::endl;
        imgGrabber.displayImage(true);

    }

}

void realSpeckleDetectionTest()
{ 
    boost::property_tree::ptree cfgParams;
    read_info("speckNullConfig.info", cfgParams);
    SpeckleNuller speckNull(cfgParams, true);
    std::chrono::microseconds rawTime, elapsedTime;
    uint64_t timestamp;
    int i;
    for(i=0; i<100; i++)
    {
        //std::string dummy;
        //std::cout << "Press any key...";
        //std::getline(std::cin, dummy);
        std::cout << std::endl;
        rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
        timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
        std::cout << "Raw TS: " << timestamp << std::endl;
        std::cout << "Starting Integration..." << std::endl;
        speckNull.updateImage(timestamp);
        elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()) - rawTime;
        timestamp = elapsedTime.count()/1000;
        std::cout << "Int time: " << timestamp << std::endl;
        speckNull.detectSpeckles();

    }

}
        


int main()
{
    boost::property_tree::ptree cfgParams;
    read_info("speckNullConfig.info", cfgParams);
    double dmA = cfgParams.get<double>("DMCal.a");
    
    std::cout << "DM A: " << dmA << std::endl;

    realSpeckleDetectionTest();


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

