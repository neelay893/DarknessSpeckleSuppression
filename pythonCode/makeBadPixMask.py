import numpy as np
import os, sys, time, struct
import matplotlib.pyplot as plt

def getBeammappedPixels(beammapFile, xSize=80, ySize=125):
    '''
    Returns a binary image: 1 if not beammapped, else 0
    '''
    beammap = np.loadtxt(beammapFile)
    rawIm = np.ones((xSize, ySize))
    beammappedInds = np.where(beammap[:,1]==0)[0]
    beammappedXLocs = np.array(beammap[beammappedInds,2], dtype=np.int32)
    beammappedYLocs = np.array(beammap[beammappedInds,3], dtype=np.int32)
    rawIm[beammappedXLocs, beammappedYLocs] = 0
    return rawIm
    
def saveBinImg(image, imageFn):
    print 'saving darkness image at', imageFn
    darkImArr = image.flatten()
    darkImStr = struct.pack('{}{}'.format(len(darkImArr), 'H'), *darkImArr)
    imgFile = open(imageFn, 'wb')
    imgFile.write(darkImStr)
    imgFile.close()
    
if __name__=='__main__':
    mdd = os.environ['MKID_DATA_DIR']
    bmdir = os.path.join(mdd, "Beammap")
    beammapFn = "finalMap_20170914.txt"
    badPixImgFn = "badPixelMask.img"
    
    beammapFile = os.path.join(bmdir, beammapFn)
    badPixMask = np.transpose(getBeammappedPixels(beammapFile)) #transpose to agree w/ opencv convention
    plt.imshow(badPixMask)
    plt.show()
    saveBinImg(badPixMask, os.path.join(mdd, badPixImgFn))
