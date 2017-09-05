import numpy as np
import os, sys, time, struct

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
