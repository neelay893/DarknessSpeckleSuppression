#include "Speckle.h"

Speckle::Speckle(cv::Point2i &pt, unsigned int intensity)
{
    coordinates = pt;
    for(int i=0; i<NPHASES; i++)
    {
        phaseList[i] = (double)2*M_PI*i/NPHASES;
        intensities[i] = 0;

    }
    apertureMask = cv::Mat::zeros(2*SPECKLEAPERTURERADIUS+1, 2*SPECKLEAPERTURERADIUS+1, CV_16UC1);
    cv::circle(apertureMask, cv::Point(SPECKLEAPERTURERADIUS, SPECKLEAPERTURERADIUS), SPECKLEAPERTURERADIUS, 1, -1);

}

unsigned short Speckle::measureSpeckleIntensity(cv::Mat &image)
{
    cv::Mat speckleIm = cv::Mat(image, cv::Range(coordinates.y-SPECKLEAPERTURERADIUS, coordinates.y+SPECKLEAPERTURERADIUS+1),
                            cv::Range(coordinates.x-SPECKLEAPERTURERADIUS, coordinates.x+SPECKLEAPERTURERADIUS+1));
    speckleIm = speckleIm.mul(apertureMask);
    std::cout << speckleIm << std::endl;
    return (unsigned short)cv::sum(speckleIm)[0];

}
