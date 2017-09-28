#include "P3KCom.h"

P3KCom::P3KCom(boost::property_tree::ptree &pt)
{
    cfgParams = pt;
    dmActMapArr = new char[2*DM_SIZE*DM_SIZE];

    flatmapDir = cfgParams.get<std::string>("P3KParams.flatmapDir");
    curFlatmap = cv::Mat::zeros(DM_SIZE, DM_SIZE, CV_64FC1);
    loadDMActuatorMap();
    
    if(cfgParams.get<bool>("P3KParams.useCentoffs"))
    {
        centoffsDir = cfgParams.get<std::string>("P3KParams.centOffsDir");
        influenceMatrixArr = new char[8*N_CENTOFFS*N_DM_ACTS];
        loadInfluenceMatrix();
        curCentoffs = cv::Mat::zeros(N_CENTOFFS, 1, CV_64FC1);
    
    }
    
    if(cfgParams.get<bool>("P3KParams.useSSH"))
        startSSHSession();

}

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
        
void P3KCom::loadNewCentoffsFromFlatmap(cv::Mat &flatmap, std::string centoffsFn)
{
    cv::Mat centoffs = convertFlatmapToCentoffs(flatmap, influenceMatrix);
    if(cfgParams.get<bool>("P3KParams.clampCentoffs"))
        centoffs = clampCentoffs(centoffs);
    
    centoffs += curCentoffs;
    
    double centoffVal;
    std::ofstream centoffsFile(centoffsDir + centoffsFn, std::ofstream::trunc);
    for(int r=0; r<centoffs.rows; r++)
    {
        centoffVal = centoffs.at<double>(r);
        centoffsFile << centoffVal << ' ';

    }
    
    centoffsFile.close();

}

/**
 * Grabs current centroid offsets from text file, 
 * and saves in curCentoffs Mat
 **/
void P3KCom::grabCurrentCentoffs(std::string centoffsFn)
{
    std::ifstream centoffsFile(centoffsDir + centoffsFn, std::ifstream::in);
    std::string val;
    int centoffsInd = 0;
    while(centoffsFile)
    {
        centoffsFile >> val;
        curCentoffs.at<double>(centoffsInd, 1) = std::stod(val);
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
    std::ifstream flatmapFile(flatmapDir + flatmapFn, std::ifstream::in);
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
        
    
    
    std::ofstream flatmapFile(flatmapDir + flatmapFn, std::ofstream::trunc);
    for(int i=0; i<N_DM_ACTS; i++)
    {
        flatmapFile << flatmapArr[i] << std::endl;

    }
    
    flatmapFile.close();
    
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
    std::string influenceMatFn = cfgParams.get<std::string>("ImgParams.influenceMatFile");
    std::ifstream influenceMatFile(influenceMatFn.c_str(), std::ifstream::in|std::ifstream::binary);
    influenceMatFile.read(influenceMatrixArr, 8*IMXSIZE*IMYSIZE);
    influenceMatrix = cv::Mat(IMYSIZE, IMXSIZE, CV_64FC1, influenceMatrixArr);

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
    
    char buffer[2048];
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
    
}

void P3KCom::sshApplyFlatmap(std::string flatmapFn)
{
    std::string flatmapPath = flatmapDir + flatmapFn;
    std::string sshCommand = "ao hwfp hodm_map=" + flatmapPath;
    int rc = ssh_channel_request_exec(sshChan, sshCommand.c_str());
    if(rc != SSH_OK)
    {
        ssh_channel_close(sshChan);
        ssh_channel_free(sshChan);
        ssh_disconnect(sshSesh);
        ssh_free(sshSesh);
        std::cout << "WARNING: SSH Command Error" << std::endl;
        
    }
    
}

