#include <opencv2/opencv.hpp>
#include <iostream>
#include <params.h>
#include <semaphore.h>
#include <stdlib.h>
#include <errno.h>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>

#ifndef IMAGEGRABBER_H
#define IMAGEGRABBER_H
class ImageGrabber
{
    private:
        cv::Mat rawImageShm;
        cv::Mat ctrlRegionImageShm;
        cv::Mat ctrlRegionImageCpy;
        const char* const doneImgSemName="speckNullDoneImg";
        const char* const takeImgSemName="speckNullTakeImg";
        boost::interprocess::shared_memory_object shmImgBuffer;
        boost::interprocess::mapped_region imgBufferRegion;
        boost::interprocess::shared_memory_object shmTs;
        boost::interprocess::mapped_region tsMemRegion;
        boost::interprocess::named_semaphore *doneImgSemPtr;
        boost::interprocess::named_semaphore *takeImgSemPtr;
        
        //sem_t *doneImgSem;
        //sem_t *takeImgSem;
        char *imgArr;
        char *tsPtr;
        int xCenter;
        int yCenter;
        int xCtrlStart;
        int xCtrlEnd;
        int yCtrlStart;
        int yCtrlEnd;

    public:
        ImageGrabber(int xCent=40, int yCent=60);
        void readNextImage();
        void startIntegrating(unsigned long startts);
        cv::Mat& getCtrlRegionImageShm();
        cv::Mat& getCtrlRegionImageCpy();
        cv::Mat& getRawImageShm();
        void displayImage(bool showControlRegion=false);
        void changeCenter(int xCent, int yCent);

    private:
        void grabControlRegion();
        void copyControlRegion();
        void setCtrlRegion();

};
#endif
