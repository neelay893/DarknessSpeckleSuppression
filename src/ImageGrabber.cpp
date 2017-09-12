#include "ImageGrabber.h"

ImageGrabber::ImageGrabber(int xCent, int yCent)
{
    std::cout << "Opening Image Buffer..." << std::endl;
    shmImgBuffer = boost::interprocess::shared_memory_object(boost::interprocess::open_only, "/speckNullImgBuff", boost::interprocess::read_only);
    std::cout << "Truncating..," << std::endl;
    //shmImgBuffer.truncate(2*IMXSIZE*IMYSIZE);
    std::cout << "Mapping..." << std::endl;
    imgBufferRegion = boost::interprocess::mapped_region(shmImgBuffer, boost::interprocess::read_only);
    std::cout << "Getting address..." << std::endl;
    imgArr = (char*)imgBufferRegion.get_address();
    
    std::cout << "Opening TS Buffer..." << std::endl;
    shmTs = boost::interprocess::shared_memory_object(boost::interprocess::open_only, "/speckNullTimestamp", boost::interprocess::read_write);
    //shmTs.truncate(sizeof(unsigned long));
    tsMemRegion = boost::interprocess::mapped_region(shmTs, boost::interprocess::read_write);
    tsPtr = (uint64_t*)tsMemRegion.get_address();
    
    std::cout << "Opening Semaphores..." << std::endl;
//    sem_unlink(doneImgSemName);
//    sem_unlink(takeImgSemName);
    doneImgSemPtr = new boost::interprocess::named_semaphore(boost::interprocess::open_only_t(), doneImgSemName);
    takeImgSemPtr = new boost::interprocess::named_semaphore(boost::interprocess::open_only_t(), takeImgSemName);
//    doneImgSem = sem_open(doneImgSemName, O_CREAT);
//    takeImgSem = sem_open(takeImgSemName, O_CREAT);


    //(*doneImgSemPtr).post();

    // std::cout << "doneImgSemTW " << (*doneImgSemPtr).try_wait() << std::endl;
    // std::cout << "doneImgSemTW " << (*doneImgSemPtr).try_wait() << std::endl;
    // (*doneImgSemPtr).wait();
    // while((*doneImgSemPtr).try_wait()==true)
    //     std::cout << "doneImgSemTW" << std::endl;
    
    std::cout << "Done shared memory initialization." << std::endl;
    xCenter = xCent;
    yCenter = yCent;
    setCtrlRegion();

}

void ImageGrabber::readNextImage()
{
    (*doneImgSemPtr).wait();
    cv::Mat rawImageTp = cv::Mat(IMXSIZE, IMYSIZE, CV_16UC1, imgArr);
    cv::transpose(rawImageTp, rawImageShm);
    grabControlRegion();

}

void ImageGrabber::startIntegrating(uint64_t startts)
{
    *tsPtr = startts;
    (*takeImgSemPtr).post();

}

void ImageGrabber::grabControlRegion()
{
    ctrlRegionImageShm = cv::Mat(rawImageShm, cv::Range(yCtrlStart, yCtrlEnd), cv::Range(xCtrlStart, xCtrlEnd));

}

void ImageGrabber::copyControlRegion()
{
    cv::Mat ctrlRegionTmp = cv::Mat(rawImageShm, cv::Range(yCtrlStart, yCtrlEnd), cv::Range(xCtrlStart, xCtrlEnd));
    ctrlRegionImageCpy = ctrlRegionTmp.clone();

}

cv::Mat &ImageGrabber::getCtrlRegionImageShm()
{
    return ctrlRegionImageShm;

}

cv::Mat &ImageGrabber::getCtrlRegionImageCpy()
{
    return ctrlRegionImageCpy;

}

cv::Mat &ImageGrabber::getRawImageShm()
{
    return rawImageShm;

}

void ImageGrabber::setCtrlRegion()
{
    xCtrlStart = xCenter + XCTRLSTART;
    xCtrlEnd = xCenter + XCTRLEND;
    yCtrlStart = yCenter + YCTRLSTART;
    yCtrlEnd = yCenter + YCTRLEND;

}

void ImageGrabber::changeCenter(int xCent, int yCent)
{
    xCenter = xCent;
    yCenter = yCent;
    setCtrlRegion();

}

void ImageGrabber::displayImage(bool showControlRegion)
{
    //std::cout << "rawImageShmCtrl " << ctrlRegionImageShm << std::endl;
    cv::namedWindow("DARKNESS Sim Image", cv::WINDOW_NORMAL);
    if(showControlRegion)
        cv::imshow("DARKNESS Sim Image", 2000*ctrlRegionImageShm);
    else
        cv::imshow("DARKNESS Sim Image", 2000*rawImageShm);
    cv::waitKey(0);

}

