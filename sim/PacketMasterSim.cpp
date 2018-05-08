#include <iostream>
#include <fstream>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "params.h"
#include <stdlib.h>
#include <string>

std::string imageDir = "/home/neelay/SpeckleNulling/DarknessSpeckleSuppression/darkness_simulation/images/";
std::string imgReadyFn = imageDir + "IMG_READY";
char *imgArr;
char *tsPtr;
char *intTimePtr;

void getNextImage(int timestamp)
{
    std::string fn = imageDir + std::to_string(timestamp) +  ".img";
    std::ifstream imgFile(fn.c_str(), std::ifstream::in|std::ifstream::binary);
    imgFile.read(imgArr, 2*IMXSIZE*IMYSIZE);

}

int main()
{
    const char* const doneImgSemName = "/speckNullDoneImg";
    const char* const takeImgSemName = "/speckNullTakeImg";
    std::cout << "opening image buffer" << std::endl;
    boost::interprocess::shared_memory_object shmImgBuffer(boost::interprocess::open_or_create, "/speckNullImgBuff", boost::interprocess::read_write);
    shmImgBuffer.truncate(2*125*80);
    boost::interprocess::mapped_region imgBufferRegion(shmImgBuffer, boost::interprocess::read_write);
    imgArr = (char*)imgBufferRegion.get_address();
    
    std::cout << "opening ts buffer" << std::endl;
    boost::interprocess::shared_memory_object shmTs(boost::interprocess::open_or_create, "/speckNullTimestamp", boost::interprocess::read_write);
    shmTs.truncate(sizeof(unsigned long));
    boost::interprocess::mapped_region tsMemRegion(shmTs, boost::interprocess::read_write);
    tsPtr = (char*)tsMemRegion.get_address();

    std::cout << "opening int time buffer" << std::endl;
    boost::interprocess::shared_memory_object shmIntTime(boost::interprocess::open_or_create, "/speckNullIntTime", boost::interprocess::read_write);
    shmIntTime.truncate(sizeof(unsigned long));
    boost::interprocess::mapped_region intTimeMemRegion(shmIntTime, boost::interprocess::read_write);
    intTimePtr = (char*)intTimeMemRegion.get_address();
    
    std::cout << "initializing semaphores " << S_IWOTH << std::endl;
    sem_unlink(doneImgSemName);
    sem_unlink(takeImgSemName);
    sem_t *doneImgSem = sem_open(doneImgSemName, O_CREAT, S_IWUSR|S_IRUSR, 0);
    sem_t *takeImgSem = sem_open(takeImgSemName, O_CREAT, S_IWUSR|S_IRUSR, 0);
    if((doneImgSem == SEM_FAILED)||(takeImgSem == SEM_FAILED))
    {
        std::cout << "Semaphore creation failed!" << std::endl;
        std::cout << "errno: " << strerror(errno) << std::endl;

    }
    
    //Initialize semaphore values
    int semValTmp; //only used for initializing semaphores
    sem_getvalue(doneImgSem, &semValTmp);
    while(semValTmp > 0)
    {
        sem_trywait(doneImgSem);
        sem_getvalue(doneImgSem, &semValTmp);
        std::cout << "doneImgSem " << semValTmp << std::endl;

    }
    sem_getvalue(takeImgSem, &semValTmp);
    while(semValTmp > 0)
    {
        sem_trywait(takeImgSem);
        sem_getvalue(takeImgSem, &semValTmp);
        std::cout << "takeImgSem " << semValTmp << std::endl;

    }
    
    //Main loop
    while(1)
    {
        if(sem_trywait(takeImgSem)==0)
        {
            std::cout << "grabbing image" << std::endl;
            while(access(imgReadyFn.c_str(), F_OK == -1));
            remove(imgReadyFn.c_str());
            getNextImage(12345);
            sem_post(doneImgSem);
        
        }

        else
            std::cout << "waiting" << std::endl;

        sleep(1);

    }
    

    boost::interprocess::shared_memory_object::remove("speckNullImgBuffer");
    boost::interprocess::shared_memory_object::remove("speckNullTimestamp");
    boost::interprocess::shared_memory_object::remove("speckNullIntTime");
    sem_destroy(doneImgSem);
    sem_destroy(takeImgSem);

}
