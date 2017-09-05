#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>

void writeToKvecFile(cv::Point2i kvecs, double amplitude, double phase,  bool isFinal);
void writeDoneKvecFile();
