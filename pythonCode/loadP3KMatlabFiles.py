import numpy as np
import scipy.io
import os,sys,struct

influenceMatPath = '../p3kBinFiles/hodm_64_cold.mat'
illumMatPath = '../p3kBinFiles/recon_64x_TL_ho_alpha=0.1_illum.mat'
badHodmActsPath = '../p3kBinFiles/bad_hodm_acts_20160504.mat'

influenceMatSavePath = '../p3kBinFiles/influenceMatCold.bin'
illumMatSavePath = '../p3kBinFiles/illumMat.bin'
badHodmActsMatSavePath = '../p3kBinFiles/badHodmActs.bin'

influenceMat = scipy.io.loadmat(influenceMatPath)['imat']
illumMat = scipy.io.loadmat(illumMatPath)['scaled_illum']
badHodmActsMat = scipy.io.loadmat(badHodmActsPath)['bad_hodm_acts'].T-1

influenceMatArr = influenceMat.flatten()
influenceMatStr = struct.pack('{}{}'.format(len(influenceMatArr), 'd'), *influenceMatArr)
influenceMatFile = open(influenceMatSavePath, 'wb')
influenceMatFile.write(influenceMatStr)
influenceMatFile.close()

# illumMatArr = illumMat.flatten()
# illumMatStr = struct.pack('{}{}'.format(len(illumMatArr), 'd'), *illumMatArr)
# illumMatFile = open(illumMatSavePath, 'wb')
# illumMatFile.write(illumMatStr)
# illumMatFile.close()
# 
# badHodmActsMatArr = badHodmActsMat.flatten()
# badHodmActsMatStr = struct.pack('{}{}'.format(len(badHodmActsMatArr), 'H'), *badHodmActsMatArr)
# badHodmActsMatFile = open(badHodmActsMatSavePath, 'wb')
# badHodmActsMatFile.write(badHodmActsMatStr)
# badHodmActsMatFile.close()
