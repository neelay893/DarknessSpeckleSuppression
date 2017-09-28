#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <params.h>
#include <semaphore.h>
#include <stdlib.h>
#include <errno.h>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>

#define DONEIMGSEM "/speckNullDoneImg1"
#define TAKEIMGSEM "/speckNullTakeImg1"

#ifndef IMAGEGRABBER_H
#define IMAGEGRABBER_H
class ImageGrabber
{
    private:
        cv::Mat rawImageShm;
        cv::Mat ctrlRegionImage;
        cv::Mat badPixMask;
        cv::Mat badPixMaskCtrl;
        cv::Mat flatWeights;
        cv::Mat flatWeightsCtrl;
        boost::interprocess::shared_memory_object shmImgBuffer;
        boost::interprocess::mapped_region imgBufferRegion;
        boost::interprocess::shared_memory_object shmTs;
        boost::interprocess::mapped_region tsMemRegion;
        boost::interprocess::named_semaphore *doneImgSemPtr;
        boost::interprocess::named_semaphore *takeImgSemPtr;

        boost::property_tree::ptree cfgParams;
        
        //sem_t *doneImgSem;
        //sem_t *takeImgSem;
        uint16_t *imgArr;
        uint64_t *tsPtr;
        char *badPixArr;
        char *flatCalArr;
        int xCenter;
        int yCenter;
        int xCtrlStart;
        int xCtrlEnd;
        int yCtrlStart;
        int yCtrlEnd;

    public:
        ImageGrabber(boost::property_tree::ptree &ptree);
        ImageGrabber();
        void readNextImage();
        void startIntegrating(uint64_t startts);
        cv::Mat& getCtrlRegionImage();
        cv::Mat& getRawImageShm();
        void displayImage(bool showControlRegion=false);
        void changeCenter(int xCent, int yCent);

    private:
        void grabControlRegion();
        void copyControlRegion();
        void setCtrlRegion();
        void loadBadPixMask();
        void loadFlatCal();
        void badPixFiltCtrlRegion();
        void applyFlatCalCtrlRegion();

};
#endif
