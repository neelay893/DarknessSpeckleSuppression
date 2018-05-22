import numpy as np
import os, sys, struct

flatCalDir ='/mnt/data0/CalibrationFiles/imageStacks/PAL2017b/20171001' 
flatCalFn = 'flat_AOLabFlat.npz'
flatCalFile = os.path.join(flatCalDir, flatCalFn)
flatCalSaveDir = os.path.join(os.environ['MKID_DATA_DIR'], 'snBinFiles')
flatCalSaveFile = os.path.join(flatCalSaveDir, 'flatWeights.bin')

flatDict = np.load(flatCalFile)
flatWeights = flatDict['weights']#np.transpose(flatDict['weights'])
flatWeights = flatWeights.flatten()
flatWeights[np.isinf(flatWeights)] = 0
flatWeights[np.isnan(flatWeights)] = 0
flatWeightsStr = struct.pack('{}{}'.format(len(flatWeights), 'd'), *flatWeights)
saveFile = open(flatCalSaveFile, 'wb')
saveFile.write(flatWeightsStr)
saveFile.close()
