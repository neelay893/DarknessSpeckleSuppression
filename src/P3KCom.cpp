#include "P3KCom.h"

P3KCom::P3KCom(boost::property_tree::ptree &pt)
{
    cfgParams = pt;
    dmActMapArr = new char[2*DM_SIZE*DM_SIZE];

    flatmapDir = cfgParams.get<std::string>("P3KParams.flatmapDir");
    flatmapMntDir = cfgParams.get<std::string>("P3KParams.flatmapMntDir");
    curFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64FC1);
    loadDMActuatorMap();
    
    if(cfgParams.get<bool>("P3KParams.useCentoffs"))
    {
        centoffsDir = cfgParams.get<std::string>("P3KParams.centoffsDir");
        centoffsMntDir = cfgParams.get<std::string>("P3KParams.centoffsMntDir");
        influenceMatrixArr = new char[8*N_CENTOFFS*N_DM_ACTS];
        illumMatrixArr = new char[8*ILLUM_MAT_SIZE];
        loadInfluenceMatrix();
        loadIllumMatrix();
        curCentoffs = cv::Mat::zeros(N_CENTOFFS, 1, CV_64FC1);
    
    }
    
    if(cfgParams.get<bool>("P3KParams.useSSH"))
        startSSHSession();

}

P3KCom::P3KCom()
{}

P3KCom::~P3KCom()
{
    if(cfgParams.get<bool>("P3KParams.useSSH"))
    {
        ssh_channel_close(sshChan);
        ssh_channel_free(sshChan);
        ssh_disconnect(sshSesh);
        ssh_free(sshSesh);
        
    }
    
}
        
void P3KCom::loadNewCentoffsFromFlatmap(cv::Mat &flatmap)
{
    std::cout << "converting flatmap to list..." << std::endl;
    cv::Mat flatFlatmap = convertFlatmapToList(flatmap);
    // std::cout << "converting flatmap to centoffs..." << std::endl;
    // std::cout << "flatFlatmap shape: " << flatFlatmap.rows << " " << flatFlatmap.cols << " " << std::endl;
    // std::cout << "influenceMat shape: " << influenceMatrix.rows << " " << influenceMatrix.cols << " " << std::endl;
    cv::Mat centoffs = convertFlatmapToCentoffs(flatFlatmap, influenceMatrix);
    //std::cout << centoffs << std::endl;
    std::string centoffsFn = cfgParams.get<std::string>("P3KParams.loadCentoffsFn");
    
    centoffs += curCentoffs;
    
    if(cfgParams.get<bool>("P3KParams.useIllumMat"))
        centoffs = applyIllumMatrix(centoffs, illumMatrix);
    
    std::cout << "clamping centoffs..." << std::endl;
    if(cfgParams.get<bool>("P3KParams.clampCentoffs"))
        centoffs = clampCentoffs(centoffs, cfgParams.get<double>("P3KParams.clampVal"));
    
    double centoffVal;
    std::ofstream centoffsFile(centoffsMntDir + centoffsFn, std::ofstream::trunc);
    for(int r=0; r<centoffs.rows; r++)
    {
        centoffVal = centoffs.at<double>(r,0);
        if(r==1) std::cout << centoffVal << std::endl;
        centoffsFile << centoffVal << ' ';

    }
    
    centoffsFile.close();

    if(cfgParams.get<bool>("P3KParams.useSSH"))
        sshApplyCentoffs(centoffsFn);

}

/**
 * Grabs current centroid offsets from text file, 
 * and saves in curCentoffs Mat
 **/
void P3KCom::grabCurrentCentoffs()
{
    std::string centoffsFn = cfgParams.get<std::string>("P3KParams.grabCentoffsFn");
    std::ifstream centoffsFile(centoffsMntDir + centoffsFn, std::ifstream::in);
    std::string val;
    int centoffsInd = 0;
    while(centoffsFile)
    {
        centoffsFile >> val;
        curCentoffs.at<double>(centoffsInd, 0) = std::stod(val);
        centoffsInd++;
        
    }
    
    centoffsFile.close();
    
}

/**
 * Grabs current centroid offsets from text file, 
 * and saves to curCentoffs Mat
 **/
void P3KCom::grabCurrentFlatmap()
{
    std::string flatmapFn = cfgParams.get<std::string>("P3KParams.grabFlatmapFile");
    std::ifstream flatmapFile(flatmapMntDir + flatmapFn, std::ifstream::in);
    std::string val;
    int flatmapInd = 0;
    double flatmapArr[DM_SIZE*DM_SIZE];
    while(flatmapFile)
    {
        flatmapFile >> val;
        flatmapArr[flatmapInd] = std::stod(val);
        flatmapInd++;
        
    }
    
    flatmapFile.close();
    int actMapInd;
    for(int r=0; r<DM_SIZE; r++)
        for(int c=0; c<DM_SIZE; c++)
        {
            actMapInd = dmActMap.at<uint16_t>(r,c)-1;
            if(actMapInd!=-1)
                curFlatmap.at<double>(r,c) = flatmapArr[actMapInd];
                
            else
                curFlatmap.at<double>(r,c) = 0;
               
        }
        
    
}

cv::Mat P3KCom::convertFlatmapToList(cv::Mat &flatmap)
{
    cv::Mat flatmapList = cv::Mat::zeros(N_DM_ACTS, 1, CV_64F);
    double flatmapVal;
    int actMapInd;
    for(int r=0; r<DM_SIZE; r++)
        for(int c=0; c<DM_SIZE; c++)
        {
            actMapInd = dmActMap.at<uint16_t>(r,c)-1;
            if(actMapInd!=-1)
            {
                flatmapVal = flatmap.at<double>(r,c);
                flatmapList.at<double>(actMapInd,0) = flatmapVal;
                
            }
        
        }

    return flatmapList;

}

/*
 * Writes a txt file containing flatmap offsets described by <flatmap>
 * Actual flatmap is curFlatmap + flatmap
 */
void P3KCom::loadNewFlatmap(cv::Mat &flatmap)
{   
    std::string flatmapFn = cfgParams.get<std::string>("P3KParams.loadFlatmapFile");
    cv::Mat newFlatmap = curFlatmap + flatmap;
    int actMapInd;
    double flatmapArr[N_DM_ACTS]={0};
    double flatmapVal;
    for(int r=0; r<DM_SIZE; r++)
        for(int c=0; c<DM_SIZE; c++)
        {
            actMapInd = dmActMap.at<uint16_t>(r,c)-1;
            if(actMapInd!=-1)
            {
                flatmapVal = newFlatmap.at<double>(r,c);
                flatmapArr[actMapInd] = flatmapVal;
                
            }
        
        }
        
    
    
    std::ofstream flatmapFile(flatmapMntDir + flatmapFn, std::ofstream::trunc);
    for(int i=0; i<N_DM_ACTS; i++)
    {
        flatmapFile << flatmapArr[i] << std::endl;

    }
    
    flatmapFile.close();
    
    std::cout << flatmapMntDir + flatmapFn << std::endl;
    
    if(cfgParams.get<bool>("P3KParams.useSSH"))
        sshApplyFlatmap(flatmapFn);
    
}

void P3KCom::loadDMActuatorMap()
{
    std::string actMapFn = cfgParams.get<std::string>("P3KParams.actMapFile");
    std::ifstream actMapFile(actMapFn.c_str(), std::ifstream::in|std::ifstream::binary);
    actMapFile.read(dmActMapArr, 2*DM_SIZE*DM_SIZE);
    dmActMap = cv::Mat(DM_SIZE, DM_SIZE, CV_16UC1, dmActMapArr);
    
}

void P3KCom::loadInfluenceMatrix()
{
    std::string influenceMatFn = cfgParams.get<std::string>("P3KParams.influenceMatFile");
    std::ifstream influenceMatFile(influenceMatFn.c_str(), std::ifstream::in|std::ifstream::binary);
    influenceMatFile.read(influenceMatrixArr, 8*N_CENTOFFS*N_DM_ACTS);
    influenceMatrix = cv::Mat(N_CENTOFFS, N_DM_ACTS, CV_64FC1, influenceMatrixArr);

}

void P3KCom::loadIllumMatrix()
{
    std::string illumMatFn = cfgParams.get<std::string>("P3KParams.illumMatFile");
    std::ifstream illumMatFile(illumMatFn.c_str(), std::ifstream::in|std::ifstream::binary);
    illumMatFile.read(illumMatrixArr, 8*ILLUM_MAT_SIZE);
    illumMatrix = cv::Mat(ILLUM_MAT_SIZE, 1, CV_64FC1, illumMatrixArr);

}
    
/**
 * starts ssh session with P3K telem
 */
void P3KCom::startSSHSession()
{
    sshSesh = ssh_new();
    if(sshSesh == NULL)
        exit(-1);

    int verbosity = SSH_LOG_PROTOCOL;
    int port = 22;

    ssh_options_set(sshSesh, SSH_OPTIONS_HOST, "aousr@p3k-telem.palomar.caltech.edu");
    ssh_options_set(sshSesh, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
    ssh_options_set(sshSesh, SSH_OPTIONS_PORT, &port);
    
    char buffer[25600];
    int nbytes;
    int rc;

    rc = ssh_connect(sshSesh);
    if(rc != SSH_OK)
    {
        fprintf(stderr, "Error connecting: %s\n", ssh_get_error(sshSesh));
        exit(-1);

    }

    int state = ssh_is_server_known(sshSesh);

    if(state==SSH_SERVER_KNOWN_OK)
        printf("server is known");

    rc = ssh_userauth_publickey_auto(sshSesh, NULL, NULL);
    if(rc != SSH_AUTH_SUCCESS)
    {
        fprintf(stderr, "password problems %s\n", ssh_get_error(sshSesh));
        ssh_disconnect(sshSesh);
        ssh_free(sshSesh);
        exit(-1);

    }
    else
        printf("Authentication success!\n");

    sshChan = ssh_channel_new(sshSesh);
    rc = ssh_channel_open_session(sshChan);
    if(rc != SSH_OK)
    {
        ssh_channel_free(sshChan);
        exit(-1);

    }

    rc = ssh_channel_request_exec(sshChan, "ls -l");
    if(rc != SSH_OK)
    {
        ssh_channel_close(sshChan);
        ssh_channel_free(sshChan);
        ssh_disconnect(sshSesh);
        ssh_free(sshSesh);
        std::cout << "WARNING: SSH Command Error" << std::endl;
        
    }
    
    nbytes = ssh_channel_read(sshChan, buffer, sizeof(buffer), 0);
    
    while(nbytes > 0)
    {
        //std::cout << buffer << std::endl;
        nbytes = ssh_channel_read(sshChan, buffer, sizeof(buffer), 0);
        
    }
    
    sshChan = ssh_channel_new(sshSesh);
    rc = ssh_channel_open_session(sshChan);
    if(rc != SSH_OK)
    {
        ssh_channel_free(sshChan);
        exit(-1);

    }
    rc = ssh_channel_request_exec(sshChan, "ls -l");
    if(rc != SSH_OK)
    {
        ssh_channel_close(sshChan);
        ssh_channel_free(sshChan);
        ssh_disconnect(sshSesh);
        ssh_free(sshSesh);
        std::cout << "WARNING: SSH Command Error" << std::endl;
        
    }
    
    ssh_channel_read(sshChan, buffer, sizeof(buffer), 0);
    //std::cout << buffer << std::endl;
    std::cout << "SSH Channel Opened" << std::endl;
    
}

void P3KCom::sshApplyFlatmap(std::string flatmapFn)
{
    char *buffer = new char[2048];
    std::string flatmapPath = flatmapDir + flatmapFn;
    std::string sshCommand = "ao hwfp hodm_map=" + flatmapPath;
    sshSendCommand(sshCommand);

}

void P3KCom::sshApplyCentoffs(std::string centoffsFn)
{
    char *buffer = new char[2048];
    std::string centoffsPath = centoffsDir + centoffsFn;
    std::string sshCommand = "ao hwfp cent_offsets=" + centoffsPath;
    sshSendCommand(sshCommand);

}

void P3KCom::sshSendCommand(std::string sshCommand)
{
    char *buffer = new char[2048];
    std::cout << "SSH Command" << sshCommand << std::endl;
    std::cout << "Creating ssh Channel..." << std::endl;
    sshChan = ssh_channel_new(sshSesh);
    std::cout << "Opening ssh channel..." << std::endl;
    int rc = ssh_channel_open_session(sshChan);
    if(rc != SSH_OK)
    {
        ssh_channel_close(sshChan);
        ssh_channel_free(sshChan);
        ssh_disconnect(sshSesh);
        ssh_free(sshSesh);
        std::cout << "Failure opening ssh channel" << std::endl;
        exit(-1);

    }
    std::cout << "Sending ssh command..." << std::endl;
    std::cout << "SSHCommand C Str.." << sshCommand.c_str() << std::endl;
    //sshCommand = "ls -l";
    ssh_channel_request_exec(sshChan, sshCommand.c_str());
    if(rc != SSH_OK)
    {
        ssh_channel_close(sshChan);
        ssh_channel_free(sshChan);
        ssh_disconnect(sshSesh);
        ssh_free(sshSesh);
        std::cout << "WARNING: SSH Command Error (applying flatmap)" << std::endl;
        
    }
    
    else
    {
        ssh_channel_read(sshChan, buffer, sizeof(buffer), 0);
        std::cout << buffer << std::endl;
        std::cout << "command applied!" << std::endl;
    
    }
    
    ssh_channel_close(sshChan);
    
}

