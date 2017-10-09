#include "ImageGrabber.h"

ImageGrabber::ImageGrabber(boost::property_tree::ptree &ptree)
{
    cfgParams = ptree;
    std::cout << "Opening Image Buffer..." << std::endl;
    shmImgBuffer = boost::interprocess::shared_memory_object(boost::interprocess::open_only, "/speckNullImgBuff", boost::interprocess::read_only);
    std::cout << "Truncating..," << std::endl;
    //shmImgBuffer.truncate(2*IMXSIZE*IMYSIZE);
    std::cout << "Mapping..." << std::endl;
    imgBufferRegion = boost::interprocess::mapped_region(shmImgBuffer, boost::interprocess::read_only);
    std::cout << "Getting address..." << std::endl;
    imgArr = (uint16_t*)imgBufferRegion.get_address();
    
    std::cout << "Opening TS Buffer..." << std::endl;
    shmTs = boost::interprocess::shared_memory_object(boost::interprocess::open_only, "/speckNullTimestamp", boost::interprocess::read_write);
    //shmTs.truncate(sizeof(unsigned long));
    tsMemRegion = boost::interprocess::mapped_region(shmTs, boost::interprocess::read_write);
    tsPtr = (uint64_t*)tsMemRegion.get_address();
    
    std::cout << "Opening Semaphores..." << std::endl;
//    sem_unlink(doneImgSemName);
//    sem_unlink(takeImgSemName);
    doneImgSemPtr = new boost::interprocess::named_semaphore(boost::interprocess::open_only_t(), DONEIMGSEM);
    takeImgSemPtr = new boost::interprocess::named_semaphore(boost::interprocess::open_only_t(), TAKEIMGSEM);
//    doneImgSem = sem_open(doneImgSemName, O_CREAT);
//    takeImgSem = sem_open(takeImgSemName, O_CREAT);


    std::cout << "Done shared memory initialization." << std::endl;
    xCenter = cfgParams.get<int>("ImgParams.xCenter");
    yCenter = cfgParams.get<int>("ImgParams.yCenter");
    setCtrlRegion();

    if(cfgParams.get<bool>("ImgParams.useBadPixMask"))
    {
        badPixArr = new char[2*IMXSIZE*IMYSIZE];
        loadBadPixMask();

    }

    if(cfgParams.get<bool>("ImgParams.useFlatCal"))
    {
        flatCalArr = new char[8*IMXSIZE*IMYSIZE]; //pack flat cal into 64 bit double array
        loadFlatCal();

    }
    
    if(cfgParams.get<bool>("ImgParams.useDarkSub"))
    {
        darkSubArr = new char[2*IMXSIZE*IMYSIZE]; //pack dark into ushort array
        loadDarkSub();

    }

}

ImageGrabber::ImageGrabber()
{}

void ImageGrabber::readNextImage()
{
    (*doneImgSemPtr).wait();
    //cv::Mat rawImageTp = cv::Mat(IMXSIZE, IMYSIZE, CV_16UC1, imgArr);
    //cv::transpose(rawImageTp, rawImageShm); 
    //std::cout << "Creating OpenCV object..." << std::endl;
    //std::cout << badPixMask << std::endl;
    rawImageShm = cv::Mat(cv::Size(IMXSIZE, IMYSIZE), CV_16UC1, imgArr);
    copyControlRegion();
    if(cfgParams.get<bool>("ImgParams.useDarkSub"))
        applyDarkSubCtrlRegion();
    if(cfgParams.get<bool>("ImgParams.useFlatCal"))
        applyFlatCalCtrlRegion();
    if(cfgParams.get<bool>("ImgParams.useBadPixMask"))
        badPixFiltCtrlRegion();

}

void ImageGrabber::startIntegrating(uint64_t startts)
{
    *tsPtr = startts;
    (*takeImgSemPtr).post();

}

void ImageGrabber::grabControlRegion()
{
    ctrlRegionImage = cv::Mat(rawImageShm, cv::Range(yCtrlStart, yCtrlEnd), cv::Range(xCtrlStart, xCtrlEnd));

}

void ImageGrabber::copyControlRegion()
{
    cv::Mat ctrlRegionTmp = cv::Mat(rawImageShm, cv::Range(yCtrlStart, yCtrlEnd), cv::Range(xCtrlStart, xCtrlEnd));
    ctrlRegionImage = ctrlRegionTmp.clone();

}

cv::Mat &ImageGrabber::getCtrlRegionImage()
{
    return ctrlRegionImage;

}

cv::Mat &ImageGrabber::getRawImageShm()
{
    return rawImageShm;

}

void ImageGrabber::setCtrlRegion()
{
    xCtrlStart = xCenter + cfgParams.get<int>("ImgParams.xCtrlStart");
    xCtrlEnd = xCenter + cfgParams.get<int>("ImgParams.xCtrlEnd");
    yCtrlStart = yCenter + cfgParams.get<int>("ImgParams.yCtrlStart");
    yCtrlEnd = yCenter + cfgParams.get<int>("ImgParams.yCtrlEnd");

}

void ImageGrabber::changeCenter(int xCent, int yCent)
{
    xCenter = xCent;
    yCenter = yCent;
    setCtrlRegion();
    if(cfgParams.get<bool>("ImgParams.useBadPixMask"))
        badPixMaskCtrl = cv::Mat(badPixMask, cv::Range(yCtrlStart, yCtrlEnd), cv::Range(xCtrlStart, xCtrlEnd));


}

void ImageGrabber::loadBadPixMask()
{
    std::string badPixFn = cfgParams.get<std::string>("ImgParams.badPixMaskFile");
    std::ifstream badPixFile(badPixFn.c_str(), std::ifstream::in|std::ifstream::binary);
    if(!badPixFile.good()) std::cout << "WARNING: Could not find bad pixel mask" << std::endl;
    badPixFile.read(badPixArr, 2*IMXSIZE*IMYSIZE);
    badPixMask = cv::Mat(IMYSIZE, IMXSIZE, CV_16UC1, badPixArr);
    badPixMaskCtrl = cv::Mat(badPixMask, cv::Range(yCtrlStart, yCtrlEnd), cv::Range(xCtrlStart, xCtrlEnd));

}

void ImageGrabber::loadFlatCal()
{
    std::string flatCalFn = cfgParams.get<std::string>("ImgParams.flatCalFile");
    std::ifstream flatCalFile(flatCalFn.c_str(), std::ifstream::in|std::ifstream::binary);
    if(!flatCalFile.good()) std::cout << "WARNING: Could not find flat cal" << std::endl;
    flatCalFile.read(flatCalArr, 8*IMXSIZE*IMYSIZE);
    flatWeights = cv::Mat(IMYSIZE, IMXSIZE, CV_64FC1, flatCalArr);
    flatWeightsCtrl = cv::Mat(flatWeights, cv::Range(yCtrlStart, yCtrlEnd), cv::Range(xCtrlStart, xCtrlEnd));
    //cv::imshow("flat", flatWeights);
    //cv::waitKey(0);
    //std::cout << flatWeights << std::endl;

}

void ImageGrabber::loadDarkSub()
{
    std::string darkSubFn = cfgParams.get<std::string>("ImgParams.darkSubFile");
    std::ifstream darkSubFile(darkSubFn.c_str(), std::ifstream::in|std::ifstream::binary);
    if(!darkSubFile.good()) std::cout << "WARNING: Could not find dark sub" << std::endl;
    darkSubFile.read(darkSubArr, 2*IMXSIZE*IMYSIZE);
    darkSub = cv::Mat(IMYSIZE, IMXSIZE, CV_16UC1, darkSubArr);
    darkSubCtrl = cv::Mat(darkSub, cv::Range(yCtrlStart, yCtrlEnd), cv::Range(xCtrlStart, xCtrlEnd));

}

void ImageGrabber::badPixFiltCtrlRegion()
{
    cv::Mat badPixMaskCtrlInv = (~badPixMaskCtrl)&1;
    cv::Mat ctrlRegionFilt = ctrlRegionImage.clone();
    cv::medianBlur(ctrlRegionImage.mul(badPixMaskCtrlInv), ctrlRegionFilt, cfgParams.get<int>("ImgParams.badPixFiltSize"));
    ctrlRegionImage = ctrlRegionFilt.mul(badPixMaskCtrl) + ctrlRegionImage.mul(badPixMaskCtrlInv);

}

void ImageGrabber::applyFlatCalCtrlRegion()
{
    std::cout<<"applying flat" << std::endl;
    cv::Mat ctrlRegionImageDouble;
    ctrlRegionImage.convertTo(ctrlRegionImageDouble, CV_64FC1);
    ctrlRegionImageDouble = ctrlRegionImageDouble.mul(flatWeightsCtrl);
    ctrlRegionImageDouble.convertTo(ctrlRegionImage, CV_16UC1);
    //std::cout << ctrlRegionImage << std::endl;

}

void ImageGrabber::applyDarkSubCtrlRegion()
{
    ctrlRegionImage = ctrlRegionImage - darkSubCtrl;

}

void ImageGrabber::displayImage(bool showControlRegion)
{
    //std::cout << "rawImageShm " << rawImageShm << std::endl;
    //cv::namedWindow("DARKNESS Sim Image", cv::WINDOW_NORMAL);
    //std::cout << "badPixMaskCtrl" << badPixMaskCtrl << std::endl;
    if(showControlRegion)
    {
     //   cv::imshow("DARKNESS Sim Image", 10*ctrlRegionImage); 
        std::cout << ctrlRegionImage << std::endl;

    }
    else
        cv::imshow("DARKNESS Sim Image", 10*rawImageShm);
        //std::cout << rawImageShm << std::endl;
    //cv::waitKey(0);

}

/*
void ImageGrabber::closeShm()
{
    boost::interprocess::named_semaphore::remove(DONEIMGSEM);
    boost::interprocess::named_semaphore::remove(TAKEIMGSEM);
    boost::
}*/
