#include <opencv2/opencv.hpp>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <libssh/libssh.h>
#include "params.h"
#include "dmTools.h"
#include <boost/property_tree/ptree.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <opencv2/core/eigen.hpp>
#include <chrono>

#ifndef P3KCOMFAST_H
#define P3KCOMFAST_H

/**
 * Communicates with P3K AO system to apply DM flat maps or 
 * WFS centroid offsets. Requires P3K control computer to be mounted
 * to the computer running this code (i.e. dark, the MKID readout server).
 * Basic procedure is:
 *  1. Write flatmap/centoff file to designated directory on P3K computer
 *  2. Write shell command to P3K over ssh. SSH session is only started once
       and is kept open, so this is relatively fast.
 **/
class P3KComFast
{
    private:
        cv::Mat influenceMatrix; //P3K influence matrix (DM space -> WFS centroid space)
        Eigen::SparseMatrix<float> influenceMatrixSparse;
        cv::Mat illumMatrix; //P3K illumination matrix
        cv::Mat curCentoffs; //Stores reference centroid offsets grabbed from P3K before nulling was started
        cv::Mat curFlatmap; //Same as above but for flatmap
        cv::Mat dmActMap; //2D array, DM_SIZExDM_SIZE that maps DM actuator position to index in DM flatmap file
        
        char *dmActMapArr; //load uint16 from bin file; memory buffer for dmActMap
        char *influenceMatrixArr; //load double from bin file; memory buffer for influence matrix
        char *illumMatrixArr; //memory buffer for illumination matrix
        
        std::string flatmapMntDir; //flatmap directory as mounted on dark
        std::string flatmapDir; //flatmap directory on P3K computer (needed for SSH command)
        std::string centoffsMntDir; //same as above but for centoffs
        std::string centoffsDir;
        
        boost::property_tree::ptree cfgParams; //container for configuration parameters

        int sockfd;
        struct sockaddr_in servaddr;

        void initializeTCPConnection();

    public:
        /**
        * Constructor. Initializes influenceMatrix, dmActuatorMap, illumMat,
        * starts SSH session if that param value is set to true
        **/
        P3KComFast(boost::property_tree::ptree &pt); 
        
        P3KComFast(); //default constructor; unused


        /**
        * Loads in and applies new WFS centoffs from the specified flatmap.
        * Flatmap is specified as an offset; i.e. the flatmap 
        * is first converted to a list of centroid offsets,
        * then this list is added to curCentoffs.
        */
        void loadNewCentoffsFromFlatmap(cv::Mat &flatmap);
        void sparseConversionTest(cv::Mat &flatmap);

        /*
         * Grabs the flatmap specified by P3KParams.grabFlatmapFn
         * and stores it in curFlatmap. This should be a file containing 
         * reference WFS centroid offsets from before speckle nulling
         */
        void grabCurrentFlatmap();

        /*
         * Grabs the centroid offsets specified by P3KParams.grabCentoffsFn
         * and stores it in curCentoffs. This should be a file containing 
         * reference WFS centroid offsets from before speckle nulling
         */
        void grabCurrentCentoffs();

        /**
        * Loads in and applies a new flatmap from the specified flatmap.
        * Provided flatmap is assumed to be an offset relative to 
        * curFlatmap
        **/
        void loadNewFlatmap(cv::Mat &flatmap);

    private:
        // load these arrays from packed binary files, made by python scripts
        void loadInfluenceMatrix();
        void loadIllumMatrix();
        void loadDMActuatorMap();

        void populateSparseInfluenceMatrix();

        // starts the ssh session with the P3K control computer
        void startSSHSession();

        // sends shell commands to apply flatmaps and centoffs over ssh
        void sshApplyFlatmap(std::string flatmapFn);
        void sshApplyCentoffs(std::string centoffsFn);

        // function to send ssh command
        void sshSendCommand(std::string sshCommand);
        cv::Mat convertFlatmapToList(cv::Mat &flatmap);

};
#endif
