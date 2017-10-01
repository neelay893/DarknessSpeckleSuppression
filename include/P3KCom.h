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
        cv::Mat illumMatrix;
        cv::Mat curCentoffs;
        cv::Mat curFlatmap;
        cv::Mat dmActMap;
        
        char *dmActMapArr; //load uint16 from bin file
        char *influenceMatrixArr; //load double from bin file
        char *illumMatrixArr;
        
        std::string flatmapMntDir;
        std::string flatmapDir;
        std::string centoffsMntDir;
        std::string centoffsDir;
        
        boost::property_tree::ptree cfgParams;

        ssh_session sshSesh;
        ssh_channel sshChan;

    public:
        P3KCom(boost::property_tree::ptree &pt);
        P3KCom();
        ~P3KCom();
        void loadNewCentoffsFromFlatmap(cv::Mat &flatmap);
        void grabCurrentFlatmap();
        void grabCurrentCentoffs();
        void loadNewFlatmap(cv::Mat &flatmap);

    private:
        void loadInfluenceMatrix();
        void loadIllumMatrix();
        void loadDMActuatorMap();
        void startSSHSession();
        void sshApplyFlatmap(std::string flatmapFn);
        void sshApplyCentoffs(std::string centoffsFn);
        void sshSendCommand(std::string sshCommand);
        cv::Mat convertFlatmapToList(cv::Mat &flatmap);

};
#endif
