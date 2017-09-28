#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <libssh/libssh.h>
#include "params.h"
#include "dmTools.h"
#include <boost/property_tree/ptree.hpp>

#ifndef P3KCOM_H
#define P3KCOM_H
class P3KCom
{
    private:
        cv::Mat influenceMatrix;
        cv::Mat curCentoffs;
        cv::Mat curFlatmap;
        cv::Mat dmActMap;
        
        char *dmActMapArr; //load uint16 from bin file
        char *influenceMatrixArr; //load double from bin file
        
        std::string flatmapDir;
        std::string centoffsDir;
        
        boost::property_tree::ptree cfgParams;

        ssh_session sshSesh;
        ssh_channel sshChan;

    public:
        P3KCom(boost::property_tree::ptree &pt);
        ~P3KCom();
        void loadNewCentoffsFromFlatmap(cv::Mat &flatmap, std::string centoffsFn);
        void grabCurrentFlatmap();
        void grabCurrentCentoffs(std::string centoffsFn);
        void loadNewFlatmap(cv::Mat &flatmap);

    private:
        void loadInfluenceMatrix();
        void loadDMActuatorMap();
        void startSSHSession();
        void sshApplyFlatmap(std::string flatmapFn);

};
#endif
