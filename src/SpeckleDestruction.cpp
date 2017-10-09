#include <SpeckleNuller.h>
#include <Speckle.h>
#include <ImageGrabberSim.h>
#include <ImageGrabber.h>
#include <P3KCom.h>
#include <dmTools.h>
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



void speckNullLoop()
{
    boost::property_tree::ptree cfgParams;
    read_info("speckNullConfig.info", cfgParams);
    SpeckleNuller speckNull(cfgParams, true);
    std::vector<ImgPt> imgPts;
    std::chrono::microseconds rawTime;
    uint64_t timestamp;

    for(int n=0; n<10; n++)
    {
        std::cout << "================BEGIN LOOP ITERATION=================" << std::endl;
        rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
        timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
        std::cout << "Raw TS: " << timestamp << std::endl;
        std::cout << "Starting Integration..." << std::endl;
        speckNull.updateImage(timestamp+200);
        //speckNull.updateImage(0);
        std::cout << "Detecting Speckles..." << std::endl;
        imgPts = speckNull.detectSpeckles();
        std::cout << "Creating Speckle Objects..." << std::endl;
        speckNull.createSpeckleObjects(imgPts);
        std::cout << "Creating Probe Speckles..." << std::endl;
        for(int i=0; i<4; i++)
        {
            std::cout << "------Begin Probe Iteration---------" << std::endl;
            std::cout << "generating probe flatmap " << std::endl;
            speckNull.generateProbeFlatmap(i);
            std::cout << "loading probe centoffs" << std::endl;
            speckNull.loadProbeSpeckles();
            rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
            timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
            std::cout << "Raw TS: " << timestamp << std::endl;
            std::cout << "Starting Integration..." << std::endl;
            speckNull.updateImage(timestamp+200);
            speckNull.measureSpeckleProbeIntensities(i);
            std::cout << std::endl << std::endl << std::endl;

        }

        if(cfgParams.get<bool>("NullingParams.useGainLoop"))
        {
            speckNull.calculateFinalPhases();

            for(int i=0; i<4; i++)
            {
                std::cout << "-------Begin Gain Loop Iteration-------" << std::endl;
                std::cout << "generating gain probe flatmap" << std::endl;
                speckNull.generateProbeGainFlatmap(i);
                std::cout << "loading probe centoffs" << std::endl;
                speckNull.loadProbeSpeckles();
                rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
                timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
                std::cout << "Raw TS: " << timestamp << std::endl;
                std::cout << "Starting Integration..." << std::endl;
                speckNull.updateImage(timestamp+200);
                speckNull.measureSpeckleGainIntensities(i);
                std::cout << std::endl << std::endl << std::endl;

            }

            std::cout << "Calculating optimal gain..." << std::endl;
            speckNull.calculateFinalGains();
            speckNull.generateNullingFlatmap();
            speckNull.loadNullingSpeckles();
            speckNull.clearSpeckleObjects();

        }


        else
        {
            std::cout << "Nulling Speckles..." << std::endl;
            speckNull.calculateFinalPhases();
            speckNull.generateNullingFlatmap(cfgParams.get<double>("NullingParams.defaultGain"));
            speckNull.loadNullingSpeckles();
            speckNull.clearSpeckleObjects();
        
        }
        std::cout << std::endl << std::endl << std::endl;
        
        std::string dummy;
        std::cout << "Press any key...";
        std::getline(std::cin, dummy);
        std::cout << std::endl;

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
    for(i=0; i<1; i++)
    {
        std::string dummy;
        std::cout << "Press any key...";
        std::getline(std::cin, dummy);
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

void simpleP3KTest()
{
    boost::property_tree::ptree cfgParams;
    read_info("speckNullConfig.info", cfgParams);
    
    cv::Point2d kvecs(2,2);
    double phase = 0;
    uint16_t intensity = 1000;
    
    cv::Mat flatmap = generateFlatmap(kvecs, intensity, phase);
    P3KCom p3k;
    p3k = P3KCom(cfgParams);
    p3k.grabCurrentFlatmap();
    p3k.loadNewFlatmap(flatmap);
    
}

void simpleFlatmapLoadSaveTest()
{
    boost::property_tree::ptree cfgParams;
    std::cout << "reading cfg file" << std::endl;
    read_info("speckNullConfig.info", cfgParams);
    std::cout << "generating new flatmap" << std::endl;
    cv::Mat newFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);
    std::cout << "creating p3k com object" << std::endl;
    P3KCom p3k = P3KCom(cfgParams);
    std::cout << "grabbing flatmap" << std::endl;
    p3k.grabCurrentFlatmap();
    std::cout << "loading flatmap" << std::endl;
    newFlatmap = generateFlatmap(cv::Point2d(10,0), 100, 0);
    p3k.loadNewFlatmap(newFlatmap);
    
}

void simpleCentoffsLoadSaveTest()
{
    boost::property_tree::ptree cfgParams;
    std::cout << "reading cfg file" << std::endl;
    read_info("speckNullConfig.info", cfgParams);
    std::cout << "generating new flatmap" << std::endl;
    cv::Mat newFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);
    std::cout << "creating p3k com object" << std::endl;
    //P3KCom *p3k;
    P3KCom *p3k;
    p3k = new P3KCom(cfgParams);
    std::cout << "grabbing centoffs" << std::endl;
    (*p3k).grabCurrentCentoffs();
    std::cout << "loading centoffs" << std::endl;
    cv::Point2i coords(35,0);
    std::cout << "coords: " << coords.x << " "  << coords.y << std::endl;
    cv::Point2d kvecs = calculateKVecs(coords, cfgParams);
    //kvecs = cv::Point2d(67.77,25.14);
    std::cout << "Kvecs: " << kvecs.x << " " << kvecs.y << std::endl;
    newFlatmap = generateFlatmap(kvecs, 100, 3.1415);
    cv::Point2i coords2 = cv::Point2d(70,0);
    cv::Point2d kvecs2 = calculateKVecs(coords2, cfgParams);
    cv::Mat newFlatmap2 = generateFlatmap(kvecs2, 100, 0);
    newFlatmap += newFlatmap2;
    (*p3k).loadNewCentoffsFromFlatmap(newFlatmap);    

}

int main()
{
    simpleCentoffsLoadSaveTest();
    //speckNullLoop();
    //simpleCentoffsLoadSaveTest();
    //realSpeckleDetectionTest();
    //simpleCentoffsLoadSaveTest();


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

