import numpy as np
import os, sys, struct

flatCalDir = '/mnt/data0/CalibrationFiles/tempFlatCal/LabData/20170918/'
flatCalFn = 'labFlat1_weights.npz'
flatCalFile = os.path.join(flatCalDir, flatCalFn)
flatCalSaveFn = os.path.join(flatCalDir, 'flatWeights.bin')

flatDict = np.load(flatCalFile)
flatWeights = np.transpose(flatDict['weights'])
flatWeights = flatWeights.flatten()
flatWeightsStr = struct.pack('{}{}'.format(len(flatWeights), 'd'), *flatWeights)
saveFile = open(flatCalSaveFn, 'wb')
saveFile.write(flatWeightsStr)
saveFile.close()
