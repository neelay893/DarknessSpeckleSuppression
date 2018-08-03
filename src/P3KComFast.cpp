#include "P3KComFast.h"

P3KComFast::P3KComFast(boost::property_tree::ptree &pt)
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
        populateSparseInfluenceMatrix();
        loadIllumMatrix();
        curCentoffs = cv::Mat::zeros(N_CENTOFFS, 1, CV_64FC1);
    
    }

    if(cfgParams.get<bool>("P3KParams.useTCP"))
        initializeTCPConnection();


}

P3KComFast::P3KComFast()
{}

void P3KComFast::initializeTCPConnection()
{
    const char *p3kIP = "198.202.125.206";
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd<0){
        printf("Socket creation error %s", strerror(errno));

    }

    memset(&servaddr, '0', sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8008);

    printf("errno: %s\n", strerror(errno));
    
    if(inet_pton(AF_INET, p3kIP, &servaddr.sin_addr)<=0){
        printf("IP Addr invalid\n");

    }

    printf("errno: %s\n", strerror(errno));

    if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        printf("Socket connection error: %s %d\n", strerror(errno), errno);

    }

    printf("Socket creation success!\n");

}

        
void P3KComFast::loadNewCentoffsFromFlatmap(cv::Mat &flatmap)
{
    uint64_t timestamp;
    std::chrono::microseconds rawTime;

    rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
    timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
    
    std::cout << "converting flatmap to list..." << timestamp << std::endl;
    cv::Mat flatFlatmap = convertFlatmapToList(flatmap);

    rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
    timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
    std::cout << "converting flatmap to centoffs..." << timestamp << std::endl;
    cv::Mat centoffs = convertFlatmapToCentoffsSparse(flatFlatmap, influenceMatrixSparse);

    rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
    timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
    std::cout << "Done converting TS: " << timestamp << std::endl;

    
    centoffs.convertTo(centoffs, CV_64FC1);
    centoffs += curCentoffs;

    
    if(cfgParams.get<bool>("P3KParams.useIllumMat"))
        centoffs = applyIllumMatrix(centoffs, illumMatrix);
    
    std::cout << "clamping centoffs..." << std::endl;
    if(cfgParams.get<bool>("P3KParams.clampCentoffs"))
        centoffs = clampCentoffs(centoffs, cfgParams.get<double>("P3KParams.clampVal"));

    //send things over socket
    cv::Mat centoffsFloat;
    centoffs.convertTo(centoffsFloat, CV_32F);
    char *centoffsBuff = (char*)centoffsFloat.data;
    int totalBytesSent = 0;
    int nBytesSent = 0;

    while(totalBytesSent<4*N_CENTOFFS)
    {
        printf("sending bytes...\n");
        nBytesSent = send(sockfd, centoffsBuff, 4*N_CENTOFFS-totalBytesSent, 0);
        printf("%d bytes sent\n", nBytesSent);
        if(nBytesSent==0)
        {
            printf("Broken socket!\n");
            break;

        }

        totalBytesSent += nBytesSent;
        centoffsBuff += nBytesSent;

    }


    


}

void P3KComFast::sparseConversionTest(cv::Mat &flatmap)
{
    uint64_t timestamp;
    std::chrono::microseconds rawTime;

    rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
    timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
    
    std::cout << "converting flatmap to list..." << timestamp << std::endl;
    cv::Mat flatFlatmap = convertFlatmapToList(flatmap);

    rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
    timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
    std::cout << "converting flatmap to centoffs sparse..." << timestamp << std::endl;
    cv::Mat centoffs;
    centoffs = convertFlatmapToCentoffsSparse(flatFlatmap, influenceMatrixSparse);

    rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
    timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
    std::cout << "Done converting TS: " << timestamp << std::endl;

    
    centoffs.convertTo(centoffs, CV_64F); 
    centoffs += curCentoffs;

    
    if(cfgParams.get<bool>("P3KParams.useIllumMat"))
        centoffs = applyIllumMatrix(centoffs, illumMatrix);
    
    std::cout << "clamping centoffs..." << std::endl;
    if(cfgParams.get<bool>("P3KParams.clampCentoffs"))
        centoffs = clampCentoffs(centoffs, cfgParams.get<double>("P3KParams.clampVal"));

    double centoffVal;

    std::ofstream centoffsFile("sparseCentoffsTest", std::ofstream::trunc);
    for(int r=0; r<centoffs.rows; r++)
    {
        centoffVal = centoffs.at<double>(r,0);
        if(r==1) std::cout << centoffVal << std::endl;
        centoffsFile << centoffVal << ' ';

    }
    
    centoffsFile.close();



    // old method
    rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
    timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
    
    std::cout << "converting flatmap to list..." << timestamp << std::endl;
    flatFlatmap = convertFlatmapToList(flatmap);

    rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
    timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
    std::cout << "converting flatmap to centoffs..." << timestamp << std::endl;
    centoffs = convertFlatmapToCentoffs(flatFlatmap, influenceMatrix);

    rawTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
    timestamp = rawTime.count()/500 - (uint64_t)TSOFFS*2000;
    std::cout << "Done converting TS: " << timestamp << std::endl;

    centoffs += curCentoffs;

    
    if(cfgParams.get<bool>("P3KParams.useIllumMat"))
        centoffs = applyIllumMatrix(centoffs, illumMatrix);
    
    std::cout << "clamping centoffs..." << std::endl;
    if(cfgParams.get<bool>("P3KParams.clampCentoffs"))
        centoffs = clampCentoffs(centoffs, cfgParams.get<double>("P3KParams.clampVal"));

    std::ofstream centoffsFile2("centoffsTest", std::ofstream::trunc);
    for(int r=0; r<centoffs.rows; r++)
    {
        centoffVal = centoffs.at<double>(r,0);
        if(r==1) std::cout << centoffVal << std::endl;
        centoffsFile2 << centoffVal << ' ';

    }
    
    centoffsFile2.close();

}
    

/**
 * Grabs current centroid offsets from text file, 
 * and saves in curCentoffs Mat
 **/
void P3KComFast::grabCurrentCentoffs()
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
void P3KComFast::grabCurrentFlatmap()
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

cv::Mat P3KComFast::convertFlatmapToList(cv::Mat &flatmap)
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
void P3KComFast::loadNewFlatmap(cv::Mat &flatmap)
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
    
    
}

void P3KComFast::loadDMActuatorMap()
{
    std::string actMapFn = cfgParams.get<std::string>("P3KParams.actMapFile");
    std::ifstream actMapFile(actMapFn.c_str(), std::ifstream::in|std::ifstream::binary);
    actMapFile.read(dmActMapArr, 2*DM_SIZE*DM_SIZE);
    dmActMap = cv::Mat(DM_SIZE, DM_SIZE, CV_16UC1, dmActMapArr);
    
}

void P3KComFast::loadInfluenceMatrix()
{
    std::string influenceMatFn = cfgParams.get<std::string>("P3KParams.influenceMatFile");
    std::ifstream influenceMatFile(influenceMatFn.c_str(), std::ifstream::in|std::ifstream::binary);
    influenceMatFile.read(influenceMatrixArr, 8*N_CENTOFFS*N_DM_ACTS);
    influenceMatrix = cv::Mat(N_CENTOFFS, N_DM_ACTS, CV_64FC1, influenceMatrixArr);

}

void P3KComFast::populateSparseInfluenceMatrix()
{
    int r, c;
    typedef Eigen::Triplet<float> Tpf;
    influenceMatrixSparse = Eigen::SparseMatrix<float>(N_CENTOFFS, N_DM_ACTS);
    std::vector<Tpf> tripletList; 
    int nElements = 0;
    for(r=0; r < N_CENTOFFS; r++)
        for(c=0; c < N_DM_ACTS; c++)
            if(influenceMatrix.at<double>(r,c)!=0)
                {
                    tripletList.push_back(Tpf(r, c, (float)influenceMatrix.at<double>(r,c)));
                    nElements++;

                }

    influenceMatrixSparse.setFromTriplets(tripletList.begin(), tripletList.end());
    influenceMatrixSparse.makeCompressed();
    std::cout << "nElements: " << nElements << std::endl;

}
            


void P3KComFast::loadIllumMatrix()
{
    std::string illumMatFn = cfgParams.get<std::string>("P3KParams.illumMatFile");
    std::ifstream illumMatFile(illumMatFn.c_str(), std::ifstream::in|std::ifstream::binary);
    illumMatFile.read(illumMatrixArr, 8*ILLUM_MAT_SIZE);
    illumMatrix = cv::Mat(ILLUM_MAT_SIZE, 1, CV_64FC1, illumMatrixArr);

}
    
