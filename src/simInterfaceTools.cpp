#include "simInterfaceTools.h"

std::string imageDir = "/home/neelay/SpeckleNulling/DarknessSpeckleSuppression/darkness_simulation/images/";
std::string simDir = "/home/neelay/SpeckleNulling/DarknessSpeckleSuppression/darkness_simulation/";

void writeToKvecFile(cv::Point2i kvecs, double amplitude, double phase,  bool isFinal)
{
    std::ofstream kvecFile(simDir + "kvecs.txt", std::ofstream::app);
    kvecFile << kvecs.x << " " << kvecs.y << " " << amplitude << " " << phase << " " << isFinal << " " << std::endl;
    kvecFile.close();

}

void writeDoneKvecFile()
{
    std::ofstream doneFile;
    doneFile.open(simDir + "KVECS_READY");
    doneFile.close();

}
