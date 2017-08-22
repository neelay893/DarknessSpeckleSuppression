#include <SpeckleNuller.h>
#include <iostream>

using namespace std;

int main()
{
    SpeckleNuller speckNull;
    speckNull.updateImage();
    speckNull.detectSpeckles();
    return 0;

}
