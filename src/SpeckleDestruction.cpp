#include <SpeckleNuller.h>
#include <Speckle.h>
#include <ImageGrabber.h>
#include <P3KCom.h>
#include <P3KComFast.h>
#include <dmTools.h>
#include <Calibrator.h>
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
    ImageGrabber imgGrabber(cfgParams);
    std::vector<ImgPt> imgPts;
    speckNull.updateBadPixMask(imgGrabber.getBadPixMaskCtrl());

    while(1)
    {
        imgGrabber.startIntegrating(0);
        imgGrabber.readNextImage();
        imgGrabber.processFullImage();
        speckNull.updateImage(imgGrabber.getCtrlRegionImage());
        std::cout << imgGrabber.getCtrlRegionImage() << std::endl;
        std::cout << "Detecting Speckles..." << std::endl;
        imgPts = speckNull.detectSpeckles();
        speckNull.exclusionZoneCut(imgPts);
        speckNull.updateAndCutNulledSpeckles(imgPts);
        if(cfgParams.get<bool>("TrackingParams.enforceRedetection"))
            speckNull.updateAndCutActiveSpeckles(imgPts);
        else
            speckNull.updateExistingSpeckles();
        std::cout << "Creating Speckle Objects..." << std::endl;
        speckNull.createSpeckleObjects(imgPts);
        std::cout << "Creating Probe Speckles..." << std::endl;
        for(int i=0; i<4; i++)
        {
            speckNull.generateProbeFlatmap(i);
            speckNull.generateSimProbeSpeckles(i);
            writeDoneKvecFile();
            imgGrabber.startIntegrating(0);
            imgGrabber.readNextImage();
            imgGrabber.processFullImage();
            speckNull.updateImage(imgGrabber.getCtrlRegionImage());
            speckNull.measureSpeckleProbeIntensities(i);

        }

        std::cout << "Nulling Speckles..." << std::endl;
        speckNull.updateSpecklesAndCheckNull();
        speckNull.generateNullingFlatmap();
        speckNull.generateSimFinalSpeckles();
        writeDoneKvecFile();
        if(access("QUIT", F_OK)!=-1)
           break;

    }

}

void speckNullLoop()
{
    boost::property_tree::ptree cfgParams;
    read_info("speckNullConfig.info", cfgParams);
    SpeckleNuller speckNull(cfgParams, true);
    ImageGrabber imgGrabber(cfgParams);
    std::vector<ImgPt> imgPts;
    speckNull.updateBadPixMask(imgGrabber.getBadPixMaskCtrl());
    std::chrono::microseconds rawTime;
    uint64_t timestamp;
    std::string dummy;

    for(int n=0; n<400; n++)
    {
        std::cout << "================BEGIN LOOP ITERATION=================" << std::endl;
        rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
        timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
        std::cout << "Raw TS: " << timestamp << std::endl;
        std::cout << "TS: " << (double)timestamp/2000 + TSOFFS << std::endl;
        std::cout << "Starting Integration..." << std::endl;
        imgGrabber.startIntegrating(0);
        imgGrabber.readNextImage();
        imgGrabber.processFullImage();
        speckNull.updateImage(imgGrabber.getCtrlRegionImage());
        //std::cout << imgGrabber.getCtrlRegionImage() << std::endl;
        std::cout << "Detecting Speckles..." << std::endl;
        imgPts = speckNull.detectSpeckles();
        speckNull.exclusionZoneCut(imgPts);
        speckNull.updateAndCutNulledSpeckles(imgPts);
        if(cfgParams.get<bool>("TrackingParams.enforceRedetection"))
            speckNull.updateAndCutActiveSpeckles(imgPts);
        else
            speckNull.updateExistingSpeckles();
        std::cout << "Creating Speckle Objects..." << std::endl;
        speckNull.createSpeckleObjects(imgPts);
        std::cout << "Creating Probe Speckles..." << std::endl;

        //std::cout << "Press any key...";
        //std::getline(std::cin, dummy);

        for(int i=0; i<4; i++)
        {
            std::cout << "------Begin Probe Iteration---------" << std::endl;
            speckNull.generateProbeFlatmap(i);
            rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
            timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
            std::cout << "loading probe centoffs, raw TS: " << timestamp << std::endl;
            speckNull.loadProbeSpeckles();
            rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
            timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
            std::cout << "Raw TS: " << timestamp << std::endl;
            std::cout << "TS: " << (double)timestamp/2000 << std::endl;
            std::cout << "Starting Integration..." << std::endl;
            imgGrabber.startIntegrating(0);
            imgGrabber.readNextImage();
            imgGrabber.processFullImage();
            speckNull.updateImage(imgGrabber.getCtrlRegionImage());
            speckNull.measureSpeckleProbeIntensities(i);
            std::cout << std::endl << std::endl << std::endl;

            //std::cout << "Press any key...";
            //std::getline(std::cin, dummy);

        }

        std::cout << "Nulling Speckles..." << std::endl;
        speckNull.updateSpecklesAndCheckNull();
        speckNull.generateNullingFlatmap();
        speckNull.loadNullingSpeckles();
        //std::cout << "Press any key...";
        //std::getline(std::cin, dummy);
        if(access("QUIT", F_OK)!=-1)
           break;

    }

}

void calibrateCenterAndAngle()
{ 
    boost::property_tree::ptree cfgParams;
    double integrationTime = 1000; //integration time in ms
    read_info("speckNullConfig.info", cfgParams);
    P3KCom *p3k;
    p3k = new P3KCom(cfgParams);
    (*p3k).grabCurrentCentoffs();
    Calibrator calibrator(cfgParams, p3k);
    ImageGrabber imgGrabber(cfgParams);
    calibrator.updateBadPixMask(imgGrabber.getBadPixMask());

    //Specify kvecs/coordinates here
    cv::Point2d kvecs(70,0);
    //cv::Point2d kvecs2(75,50);
    calibrator.addCalSpeckle(kvecs, 100);
    //calibrator.addCalSpeckle(kvecs2, 50);
    
    calibrator.loadCalFlatmap();
    imgGrabber.startIntegrating(0);
    imgGrabber.readNextImage();
    calibrator.updateImage(imgGrabber.getFullImage());
    calibrator.centroidCalSpeckles();
    calibrator.measureCalSpeckleIntensities();
    calibrator.determineImgCenter();
    calibrator.determineDMAngle();
    calibrator.determineLambdaOverD();

    std::cout << "Center: " << calibrator.imgCenter.x << " " << calibrator.imgCenter.y << std::endl;
    std::cout << "Angle: " << calibrator.dmAngle << std::endl;
    std::cout << "l/D: " << calibrator.lambdaOverD << std::endl;

}

    

/*
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

}*/

/*
void speckNullLoopKF() //JLlop: implementation of a Kalman Filter for better convergence into a SN solution. Based on the code by Yinzi Xin at Caltech
{
    boost::property_tree::ptree cfgParams;
    read_info("speckNullConfig.info", cfgParams);
    SpeckleNuller speckNull(cfgParams, true);
    std::vector<ImgPt> imgPts;
    std::chrono::microseconds rawTime;
    uint64_t timestamp;
    //for(int n=0; n<10; n++){	//start loop for DH digging
    rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
    timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
    std::cout << "Raw TS: " << timestamp << std::endl;
    std::cout << "Starting Integration..." << std::endl;
    speckNull.updateImage(timestamp+200);
    std::cout << "Detecting Speckles..." << std::endl;
    imgPts = speckNull.detectSpeckles();
	
    std::cout << "Creating Speckle Objects..." << std::endl;
    speckNull.createSpeckleObjects(imgPts);

    speckNull.init_KF(); //JLlop

    for(int II=0; II<5; II++) //KF loop
    {
        std::cout << "================BEGIN LOOP ITERATION=================" << std::endl;
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
	speckNull.calculateFinalPhases();
	// JLlop: So far, everything is the same as regular Speckle Nulling. Now KF:
	speckNull.calculateFinalAmplitude(); //New function to compute the amplitude on the DM corresponding to each speckle
	speckNull.updateEstimatesKF();  //Given the measurement of phase and amplitude, update the estimates of the KF
	speckNull.updateControlKF(); 	//Update finalAmplitude and finalPhase with new estimate from the KF
	// JLlop: now that FinalPhase and FinalAmplitude are updated, we can update the DM shape:
	speckNull.generateNullingFlatmap(); 

    }
    std::cout << std::endl << std::endl << std::endl;
       
    std::string dummy;
    std::cout << "Press any key...";
    std::getline(std::cin, dummy);
    std::cout << std::endl;

    //}//end of DH digging iteration loop

}*/

void realImgGrabTest()
{ 
    boost::property_tree::ptree cfgParams;
    read_info("speckNullConfig.info", cfgParams);
    ImageGrabber imgGrabber(cfgParams);
    std::chrono::microseconds rawTime, rawTime2;
    uint64_t timestamp;
    while(1)
    {
        std::string dummy;
        std::cout << "Press any key...";
        std::getline(std::cin, dummy);
        std::cout << std::endl;
        rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
        timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
        //std::cout << "Raw TS: " << timestamp << std::endl;
        //std::cout << "Starting Integration..." << std::endl;
        imgGrabber.startIntegrating(0);
        imgGrabber.readNextImage();
        rawTime2 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
        std::cout << "Real integration time: " << (rawTime2.count()/500 - rawTime.count()/500) << std::endl;
        std::cout << "Displaying Image..." << std::endl;
        imgGrabber.displayImage(false);

    }

}

/*void realSpeckleDetectionTest()
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

}*/

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
    P3KComFast *p3k;
    p3k = new P3KComFast(cfgParams);
    std::cout << "grabbing centoffs" << std::endl;
    (*p3k).grabCurrentCentoffs();
    std::cout << "loading centoffs" << std::endl;

    cv::Point2i coords(21,16);
    std::cout << "coords: " << coords.x << " "  << coords.y << std::endl;
    cv::Point2d kvecs = calculateKVecs(coords, cfgParams);
    //kvecs = cv::Point2d(67.77,25.14);
    std::cout << "Kvecs: " << kvecs.x << " " << kvecs.y << std::endl;
    //newFlatmap = generateFlatmap(kvecs, 120, 0);
    newFlatmap = generateFlatmap(cv::Point2d(70,30), 120, 3.1415);
    //cv::Point2i coords2 = cv::Point2d(70,0);
    //cv::Point2d kvecs2 = calculateKVecs(coords2, cfgParams);
    //cv::Mat newFlatmap2 = generateFlatmap(cv::Point2d(0,75), 120, 3.1415);
    //newFlatmap += newFlatmap2;
    (*p3k).loadNewCentoffsFromFlatmap(newFlatmap);    

}

void centoffsSparseTest()
{
    boost::property_tree::ptree cfgParams;
    std::cout << "reading cfg file" << std::endl;
    read_info("speckNullConfig.info", cfgParams);
    std::cout << "generating new flatmap" << std::endl;
    cv::Mat newFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64F);
    std::cout << "creating p3k com object" << std::endl;
    //P3KCom *p3k;
    P3KComFast *p3k;
    p3k = new P3KComFast(cfgParams);
    std::cout << "grabbing centoffs" << std::endl;
    (*p3k).grabCurrentCentoffs();
    std::cout << "loading centoffs" << std::endl;

    cv::Point2i coords(21,16);
    std::cout << "coords: " << coords.x << " "  << coords.y << std::endl;
    cv::Point2d kvecs = calculateKVecs(coords, cfgParams);
    //kvecs = cv::Point2d(67.77,25.14);
    std::cout << "Kvecs: " << kvecs.x << " " << kvecs.y << std::endl;
    //newFlatmap = generateFlatmap(kvecs, 120, 0);
    newFlatmap = generateFlatmap(cv::Point2d(70,30), 120, 3.1415);
    //cv::Point2i coords2 = cv::Point2d(70,0);
    //cv::Point2d kvecs2 = calculateKVecs(coords2, cfgParams);
    //cv::Mat newFlatmap2 = generateFlatmap(cv::Point2d(0,75), 120, 3.1415);
    //newFlatmap += newFlatmap2;
    (*p3k).sparseConversionTest(newFlatmap);    

}

int main()
{
    //centoffsSparseTest();
    //simpleCentoffsLoadSaveTest();
    //calibrateCenterAndAngle();
    speckNullLoop();
    //realImgGrabTest();
    //simpleCentoffsLoadSaveTest();
    //realSpeckleDetectionTest();
    //simpleCentoffsLoadSaveTest();
    //speckNullSimLoop();

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

